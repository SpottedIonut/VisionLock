# For running inference on the TF-Hub module.
import tensorflow as tf
import tensorflow_hub as hub

# For downloading the image.
import matplotlib.pyplot as plt
import tempfile
from six.moves.urllib.request import urlopen
from six import BytesIO

# For drawing onto the image.
import numpy as np
from PIL import Image
from PIL import ImageColor
from PIL import ImageDraw
from PIL import ImageFont
from PIL import ImageOps

# For measuring the inference time.
import time

# For Firebase database
import firebase_admin
import datetime
from firebase_admin import credentials, storage

# for server
from flask import Flask, request
import threading
import os

app = Flask(__name__)

# Path to your service account key JSON file
cred_path = 'summer-school-d63f0-firebase-adminsdk-ka7bk-2f493a5996.json'

# Initialize the app with a service account, granting admin privileges
cred = credentials.Certificate(cred_path)
firebase_admin.initialize_app(cred, {
    'storageBucket': 'summer-school-d63f0.appspot.com'  # replace with your storage bucket poate trb fara gs://
})

# Create a lock
lock = threading.Lock()

# Print Tensorflow version
print(tf.__version__)

# Check available GPU devices.
print("The following GPU devices are available: %s" % tf.test.gpu_device_name())


def display_image(image):
    # Convert the TensorFlow image to a PIL image
    pil_image = Image.fromarray(np.uint8(image))
    pil_image.save('result.jpg', format='JPEG')


def download_and_resize_image(url, new_width=256, new_height=256, display=False):
    _, filename = tempfile.mkstemp(suffix=".jpg")
    response = urlopen(url)
    image_data = response.read()
    image_data = BytesIO(image_data)
    pil_image = Image.open(image_data)
    pil_image_rgb = pil_image.convert("RGB")
    pil_image_rgb.save(filename, format="JPEG", quality=90)
    print("Image downloaded to %s." % filename)
    if display:
        display_image(pil_image)
    return filename


def draw_bounding_box_on_image(image, ymin, xmin, ymax, xmax, color, font, thickness=4, display_str_list=()):
    """Adds a bounding box to an image."""
    draw = ImageDraw.Draw(image)
    im_width, im_height = image.size
    (left, right, top, bottom) = (xmin * im_width, xmax * im_width, ymin * im_height, ymax * im_height)
    draw.line([(left, top), (left, bottom), (right, bottom), (right, top), (left, top)],
              width=thickness,
              fill=color)

    display_str_heights = [font.getbbox(ds)[3] for ds in display_str_list]
    total_display_str_height = (1 + 2 * 0.05) * sum(display_str_heights)

    if top > total_display_str_height:
        text_bottom = top
    else:
        text_bottom = top + total_display_str_height
    for display_str in display_str_list[::-1]:
        bbox = font.getbbox(display_str)
        text_width, text_height = bbox[2], bbox[3]
        margin = np.ceil(0.05 * text_height)
        draw.rectangle([(left, text_bottom - text_height - 2 * margin),
                        (left + text_width, text_bottom)],
                       fill=color)
        draw.text((left + margin, text_bottom - text_height - margin),
                  display_str,
                  fill="black",
                  font=font)
        text_bottom -= text_height - 2 * margin


def draw_boxes(image, boxes, class_names, scores, max_boxes=10, min_score=0.1):
    """Overlay labeled boxes on an image with formatted scores and label names."""
    colors = list(ImageColor.colormap.values())

    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/liberation/LiberationSansNarrow-Regular.ttf", 25)
    except IOError:
        print("Font not found, using default font.")
        font = ImageFont.load_default()

    for i in range(min(boxes.shape[0], max_boxes)):
        if scores[i] >= min_score:
            ymin, xmin, ymax, xmax = tuple(boxes[i])
            display_str = "{}: {}%".format(class_names[i].decode("ascii"), int(100 * scores[i]))
            color = colors[hash(class_names[i]) % len(colors)]
            image_pil = Image.fromarray(np.uint8(image)).convert("RGB")
            draw_bounding_box_on_image(image_pil, ymin, xmin, ymax, xmax, color, font, display_str_list=[display_str])
            np.copyto(image, np.array(image_pil))
    return image


def load_img(path):
    img = tf.io.read_file(path)
    img = tf.image.decode_jpeg(img, channels=3)
    return img


def run_detector(detector, path):
    img = load_img(path)

    converted_img = tf.image.convert_image_dtype(img, tf.float32)[tf.newaxis, ...]
    start_time = time.time()
    result = detector(converted_img)
    end_time = time.time()

    result = {key: value.numpy() for key, value in result.items()}

    print("Found %d objects." % len(result["detection_scores"]))
    print("Inference time: ", end_time - start_time)

    for (entity, score) in zip(result["detection_class_entities"], result["detection_scores"]):
        if (entity.startswith(b'Person') or entity.startswith(b'Human') or entity.startswith(b'Man') or entity.startswith(b'Woman') or entity.startswith(b'Boy') or entity.startswith(b'Girl')) and score > 0.15:
            print(entity, score)
            print("Persoana gasita")
            display_image(img)
            upload_image('result.jpg')
            break


def show_image(path):
    image = Image.open(path)
    image.show()


image_url = "http://10.41.66.155/capture"

downloaded_image_path = download_and_resize_image(image_url)

module_handle = "https://tfhub.dev/google/openimages_v4/ssd/mobilenet_v2/1"

# Firebase setup
bucket = storage.bucket()


def upload_image(path):
    storage_path = "persons/" + datetime.datetime.now().strftime("%Y.%m.%d-%H:%M:%S") + ".jpg"
    blob = bucket.blob(storage_path)
    blob.upload_from_filename(path)
    print("File uploaded to Firebase Storage.")


@app.route('/distance')
def distance():
    value = request.args.get('value')
    if value:
        print(f"Distance detected: {value} cm")
        if int(value) < 150:
            with lock:
                downloaded_image_path = download_and_resize_image(image_url)
                detector = hub.load(module_handle).signatures['default']
                run_detector(detector, downloaded_image_path)
    return "Distance received!"


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5200)
    

# detector = hub.load(module_handle).signatures['default']
# run_detector(detector, downloaded_image_path)