# Tema 1 - Loader de Executabile

Parcurg vectorul de segmente si verific daca adresa page-fault-ului corespune unuia dintre ele.
Daca gasesc segmentul, verific vectorul "data" care va contine, pentru fiecare pagina
posibila a segmentului, 0 daca aceasta nu a fost mapata inca, sau 1 in caz contrar.
Astfel, disting 2 cazuri:
1. Daca intrarea din "data" corespunzatoare paginii este 1, cauza page-fault-ului este
un acces ne-permis la memorie si dau semnalul SIGSEGV.
2. Daca este 0, mapez pagina, actualizez "data", citesc informatii din executabil, daca este cazul,
si zero-izez restul paginii/intreaga pagina in functie de unde se afla aceasta in segment. Setez
permisiunile paginii mapate cu permisiunile segmentului.
Daca parcurgerea vectorului de segmente se incheie si adresa paginii nu a fost gasita, dau semnalul
SIGSEGV, deoarece cauza page-faultului este accesarea unei bucati de memorie care nu face parte
din executabil.
