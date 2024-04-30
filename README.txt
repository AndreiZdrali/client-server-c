============ SUBSCRIBER ============
In subscriber.cpp se stabileste o conexiune TCP cu server-ul, dupa care se asteapta
input pe stdin sau socket-ul TCP. In cazul in care se primeste comanda "exit" de la
stdin este trimis un mesaj catre server pentru a-l avertiza ca clientul s-a deconectat.
In cazul in care se primeste un mesaj de la server atunci este interpretat conform
cerintei si afisat.

============ SERVER ============
Server-ul initializeaza socket-urile UDP si TCP, dupa care asteapta cereri de conectare
pe socket-ul TCP. Ca la subscriber, "exit" trimite un mesaj tuturor clientilor si inchide
server-ul. In cazul in care cererea este valida (nu exista alt client online cu acelasi
ID) atunci ii este atribuit un nou fd si adaugat in lista de clienti/marcat ca online.
La primirea unui mesaj UDP se calculeaza marimea structurii ce se va trimite clientilor
in functie de continutul payload-ului, pentru a-l putea trunchia si a nu trimite octeti
inutili. Se trimite mai intai lungimea ca sa stie clientul cat sa citeasca, dupa care
se trimite structura propriu-zisa ce contine sursa, topic-ul, tipul payload-ului si
payload-ul in sine. La primirea unui mesaj "exit" de la un client acesta este marcat
ca deconectat.
Modul prin care se verifica daca un client este abonat la un anumit topic este folosind
regex, astfel incat caracterul '*' poate fi inlocuit cu orice alt sir de caractere, pe
cand caracterul '+' poate fi inlocuit cu alt sir de caractere in afara de '/'.