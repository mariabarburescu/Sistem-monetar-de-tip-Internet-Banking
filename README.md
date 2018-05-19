# Sistem-monetar-de-tip-Internet-Banking

Conexiunea intre server si client am implementat-o socketi TCP.

~~~~~~~~~~
~ Server ~
~~~~~~~~~~
	Am definit o structura pentru a retine datele despre un user. Am citit din
fisier toti userii si i-am retinut intr-un vector. Pentru fiecare user am
initializat campurile blocat, logat cu false, numarul de loginuri gresite cu 0
si socketul pe care este logat cu -1.
	Adaug socketi de pe care primeste comenzi, inclusiv tastatura pentru a
putea oferi si serverul comanda quit. In acest caz serverul trimite un mesaj
catre toti clientii pentru a-i informa ca el se va inchide, dupa care asteapta
2 secunde pentru a se putea transmite mesajele si se inchide.
	Pentru comanda quit inchid clientul si il scot din lista de clienti de la
care asteapta serverul comenzi.
	Pentru comanda login, imparte bufferul pe care il primeste, cauta userul
dupa numarul de card si verifica pinul in cazul in care gaseste userul. Daca
credentialele sunt corecte si userul nu este deja logat in alt client, se
realizeaza loginul si trimite mesajul de succes, altfel trimite catre client
eroarea corespunzatoare.
	Pentru comanda logout cauta userul logat pe clientul de la care a primit
comanda si ii seteaza campul logat cu 0 si campul socket cu -1.
	Pentru comanda listsold cauta userul logat pe clientul de la care a primit
comanda si ii trimite clientului soldul userului.
	Pentru comanda transfer cauta indecsi userului care vrea sa transfere bani
si userului catre care vrea sa transfere bani. Verifica corectitudinea
credentialelor dupa care asteapta confirmarea de la client. Daca credentialele
nu sunt bune va trimite catre client mesajul corespunzator.

~~~~~~~~~~
~ Client ~
~~~~~~~~~~
	Citeste de la tastatura o comanda si verifica ce comanda este. Pentru
comenzile login si transfer mai asteapta sa citeasca si numarul de card si
pinul, respectiv suma de bani, dupa care trimite comanda catre server.
	In cazul in care clientul este deja logat si incearca o a doua logare,
acesta va fi informat si nu se trimite comanda catre server.
	In cazul in care clientul introduce una din comenzile: logout, listsold,
transfer fara a fi logat, acesta va fi informat de acest lucru si nu trimite
comanda catre server.
	Daca comanda este quit se va inchide serverul.
	Daca primeste de la server mesajul ca acesta se va inchide, se inchide si
clientul.
