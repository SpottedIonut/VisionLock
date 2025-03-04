# VisionLock
Acest sistem de securizare inteligentă funcționează ca o incuietoare care capturează imagini. O placă ESP32 cu senzori monitorizează prezența, iar dacă nu se identifică un card RFID autorizat, un server Python activează cealaltă placă cu cameră pentru a face o fotografie, încărcată ulterior în Firebase pentru verificare.

Proiectul funcționează pe baza unei arhitecturi distribuite, care combină două plăci ESP32 și un server Python pentru a crea un sistem inteligent de monitorizare și securizare a accesului. Etapele de functionare sunt:

Colectarea datelor:
    Prima placă ESP32 este echipată cu senzori capabili să măsoare distanța față de obiecte din jurul incuietorii. Această placă monitorizează constant mediul înconjurător și colectează date privind distanțele, identificând daca se apropie cineva de usa.

Transmiterea și analiza datelor:
  Datele colectate de placa cu senzori sunt trimise în timp real către un server Python, folosind o conexiune wireless (de exemplu, Wi-Fi). Serverul analizează aceste informații și, în funcție de valorile măsurate și de pragurile predefinite, decide dacă prezența identificată necesită o acțiune suplimentară.

Declanșarea capturii:
  Dacă analiza serverului indică prezența unui potențial acces neautorizat, acesta trimite un semnal către a doua placă ESP32, care este echipată cu un modul de cameră. Acest semnal acționează ca un trigger pentru capturarea unei imagini.

Capturarea și încărcarea imaginii:
  La primirea semnalului, placa cu cameră activează modulul său și capturează o fotografie. Imediat după captură, imaginea este procesată si se verifica cu ajutorul TensorFlow daca in imagine apare un om sau nu. Dacă apare un om, atunci imaginea este încărcată în Firebase, împreună cu metadate esențiale, cum ar fi data și ora la care a fost realizată. Aceste informații permit o verificare ulterioară a incidentului.

Prin această abordare, proiectul integrează funcționalități de monitorizare continuă, analiză în timp real și acțiuni automatizate, asigurând o soluție eficientă de securizare a accesului.
