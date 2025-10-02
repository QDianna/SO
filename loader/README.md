# Tema 3 – Loader de Executabile

Implementare de **lazy loading** pentru paginile unui executabil, cu tratarea corectă a page-fault-urilor și a permisiunilor de memorie.

## Idee
- La primul acces într-o pagină dintr-un segment mapabil, pagina este mapată dinamic, conținutul este încărcat sau zero-izat, iar permisiunile sunt setate conform segmentului.
- Accesul ilegal într-o pagină deja mapată sau în afara segmentelor valide generează **SIGSEGV**.

## Structuri și date
- **Vector de segmente**: intervale `[vaddr, vaddr + mem_size)` cu `file_offset`, `file_size`, `perm`.
- **data[page_idx]** per segment: `0` = pagina nemapată, `1` = pagina deja mapată.

## Flux la page-fault
1. Identifică segmentul care conține adresa faultului. Dacă nu există, trimite **SIGSEGV**.
2. Calculează indexul paginii în segment (`page_idx`).
3. Dacă `data[page_idx] == 1` → acces nepermis → **SIGSEGV**.
4. Dacă `data[page_idx] == 0`:
   - Alocă și mapează pagina la adresa corectă.
   - Copiază din fișier doar porțiunea acoperită de pagină dacă suprapune zona `[file_offset, file_offset + file_size)`.
   - Zero-izează restul (porțiunile din pagină care cad în afara `file_size` sau paginile din coada segmentului).
   - Setează permisiunile paginii la cele ale segmentului (`PROT_READ/WRITE/EXEC`).
   - Marchează `data[page_idx] = 1`.

## Cazuri particulare
- Pagină parțial acoperită de fișier: copiază bytes utili, zero-izează remainder.
- Pagină complet după `file_size` dar în interiorul `mem_size`: zero-izează complet.
- Acces în afara oricărui segment: **SIGSEGV** imediat.

## Semnalizare erori
- **SIGSEGV** pentru:
  - Acces într-o pagină mapată cu permisiuni insuficiente.
  - Acces la adrese din afara segmentelor executabilului.
