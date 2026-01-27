# Projekt Systemy Operacyjne. Temat 14: Bar Mleczny

**Autor:** Piotr Turzyński
**Nr albumu** 155237
**Rok akademicki** 2025/2026
**Środowisko** Linux WSL (Ubuntu 24.04.3 LTS.) / Torus (serwer uczelniany)
**Kompilator** gcc 13.3.0
**Temat:** Symulacja baru mlecznego

---

## 1. Opis projektu

Projekt jest symulacją baru mlecznego. Klienci wchodzą do baru o pojemności N (N = 1 * x1 + 2 * x2 + 3* x3 + 4 * x4, gdzie x(1-4) oznacza rozmiar stolika, 1-4 osobowe). Wybierają jedną z trzech dostępnych potraw. Płacą za nie u kasjera, który jest osobnym procesem. Następnie odbierają danie o pracownika, który także jest osobnym procesem. Po skonsumowaniu jedzenia opuszczają bar.

---
## 2. Uruchomienie oraz kończenie symulacji.
Start symulacji:
```
make
./main
```
Istnieje też opcja uruchomienia procesu menedżera 
```
./manager
```
Symulacje kończymy wykonując kombinacje klawiszy CTRL + C
Po zakończeniu istnieje możliwość podglądu logów z symulacji w pliku:
``bar_log.txt``

---
## 3. Opis poszczególnych plików.
**ipc.c**
- Zawiera funkcje inicjalizujące oraz sprzątające zasoby IPC tj. pamięć dzielona, semafory i kolejki komunikatów
- Zawiera funkcje pomocnicze do wykorzystywania poszczególnych zasobów.
- Zawiera funkcje otwierająca i zamykająca plik oraz zapisującą do niego.

**main.c**
- "Mózg" całej symulacji. Używa on funkcji do stworzenia potrzebnych nam IPC.
- Ustawiamy za pomocą klawiatury liczbę stolików x1 x2 x3 x4
- Uruchamia procesy potomne
- Obsługuje potrzebne nam sygnały i w razie potrzeby przekazuje je dalej do dzieci
- Wykorzystuje dwa wątki. Jeden generujący klientów a drugi sprzątający procesy zombie
- Sprząta po całej symulacji oraz wypisuje jej podsumowanie

**manager.c**
- Zarządza symulacją. Wysyła dwa sygnały do pracownika obsługi (worker.c):
	- SIGUSR1 powodujący dwukrotne zwiększenie stolików trzy osobowych (Możliwe jednokrotne wykorzystanie)
	- SIGUSR2 dzięki któremu możemy rezerwować stoliki w barze (Stają się niedostępne dla klientów)
- Jako pierwszy zauważa pożar (sygnał trzeci). Wysyła go do main.c, i ewakuujemy na początku klientów a potem naszych pracowników baru (Kasjer oraz pracownik obsługi)

**worker.c**
- Wydaje jedzenie klientom
- Reaguje na sygnały od menedżera (Podwaja stoliki trzy osobowe lub rezerwuje stoliki)

**cashier.c**
- Liczy utarg symulacji
- Przyjmuje płatności od klientów

**client.c**
- Wchodząc do baru losuje swój rozmiar grupy (1-3 os)
- Istnieje szansa <=5%, że nic nie zamówimy w barze. W takim wypadku wychodzimy z baru.
- Gdy wszystkie stoliki są zarezerwowane także wychodzimy z baru.
- Kontaktuje się  z kasjerem w przypadku płatności za nasze jedzenie
- Kontaktuje się z pracownikiem obsługi w celu odebrania jedzenia
- Zajmuje stolik (Dosiadając się do grupy równolicznej albo siada przy pustym stoliku)
- Proces wielowątkowy (Ilość wątków rozmiarGrupy -1). Każdy wątek jak i proces główny wybierają pozycje z menu.
- Po skonsumowaniu dania zwalniamy stolik i wychodzimy z baru.

---

## 4. Występujące problemy i ich rozwiązanie
Tworząc ten problem napotkałem się z kilkoma problemami, z tymi prostszymi do rozwiązania jak i z tymi cięższymi. Opiszę najważniejsze z nich:
1. Zostające procesy zombie w systemie, wolne ich czyszczenie.
     - Intensywne generowanie procesów przez wątek generatora spowalniało symulacje przez wiszące procesy w systemie. Czyszczenie ich pod koniec symulacji było nieefektywne, z tego powodu wprowadziłem asynchroniczny wątek sprzątający po klientach.
2. Przypisując personel do innej grupy procesów za pomocą `setpgid()` spowodowałem, że stali się nietykalni przez sygnały `ctrl + z / SIGSTP` oraz `fg / SIGCONT` co powodowało zakleszczenia po powrocie do symulacji.
	- Rozwiązałem ten problem za pomocą handlera sygnału w procesie main. Teraz przekazuję on te sygnały dalej. Dzięki temu procesy kasjera jak i pracownika są w stanie na nie reagować i nie powstaję już niebezpieczny deadlock.
3. Blokujące się kolejki komunikatów przy dużej ilości klientów
	- Semafor "strażnik" przed wysyłaniem kolejki pozwala kontrolować ile klientów może tam w jednym momencie wysłać wiadomość. Dzięki temu zawsze pozostaje miejsce na odpowiedź od kasjera czy to pracownika.
4. Race condition przy zapisie do pliku
	- Zastosowanie blokady pliku `flock()` na czas zapisu do bufora

---

## 5. Pseudokody algorytmów.
1. Algorytm szukania stolika przez klienta.
```
DOPÓKI nie znaleziono stolika (foundTable == -1):
    [Zablokuj dostęp do pamięci współdzielonej (SEM_MEMORY)]
    
    // 1. Sprawdzenie dostępności zasobów
    Policz stoliki, które nie są zarezerwowane
    JEŻELI liczba niezarezerwowanych stolików == 0:
        Zmniejsz licznik aktywnych klientów (bar->clients)
        [Odblokuj SEM_MEMORY]
        Otwórz drzwi dla grupy (zwalniając SEM_DOOR)
        ZAKOŃCZ PROCES (Kod: EXIT_NOTABLE)

    // 2. Szukanie optymalnego miejsca
    DLA KAŻDEJ pojemności i od wielkości grupy DO 4:
        DLA KAŻDEGO stolika j w barze:
            Pobierz parametry stolika (tab)
            
            // Warunek A: Dosiadanie się do istniejącej grupy
            JEŻELI grupa siedząca == wielkość grupy ORAZ wolne miejsca >= wielkość grupy:
                Zapisz ID stolika i jego parametry do logów
                Zmniejsz liczbę wolnych miejsc (freeSlots)
                Przerwij szukanie
                
            // Warunek B: Zajmowanie nowego, pustego i niezarezerwowanego stolika
            JEŻELI pojemność == i ORAZ stolik niezarezerwowany ORAZ stolik pusty:
                Zapisz ID stolika i jego parametry
                Zmniejsz liczbę wolnych miejsc i ustaw typ grupy (whoSits)
                Przerwij szukanie

    [Odblokuj dostęp do pamięci współdzielonej (SEM_MEMORY)]

    // 3. Obsługa braku miejsca i frustracji
    JEŻELI nadal nie znaleziono stolika:
        Zwiększ licznik prób (attempts)
        
        JEŻELI liczba prób >= 10:
            [Zablokuj SEM_MEMORY, zmniejsz bar->clients, odblokuj SEM_MEMORY]
            Obudź innego czekającego klienta (sem_wakeOne)
            Otwórz drzwi (zwalniając SEM_DOOR)
            ZAKOŃCZ PROCES (Kod: EXIT_FRUSTRATED)
        
        W PRZECIWNYM RAZIE:
            Zaszyj (Czekaj na SEM_SEARCH aż ktoś zwolni miejsce)
```
---
## 6. Testy
Do każdego testu użyłem po 5 stolików każdego rodzaju.
1. Zarezerwowanie wszystkich stolików w barze a po chwili zwiększenie liczby stolików trzy osobowych
Oczekiwany rezultat: Z każdą rezerwacją "tempo" baru powinno zwalniać z racji na zmniejszającą się liczbę klientów. W przypadku braku niezarezerwowanych stolików klienci powinni wchodzić do baru, sprawdzać stoliki i wychodzić. Wraz z podwojeniem stolików trzy osobowych "życie" w barze powinno powrócić i klienci z powrotem zajmować miejsca
	
	Logi po zarezerwowaniu wszystkich stolików:
	```
	[KLIENT 21804 | 2 osob] Wchodzimy do baru!
	[KLIENT 19661 | 3 osob] Wchodzimy do baru!
	[KLIENT 19655 | 3 osob] Wchodzimy do baru!
	[KLIENT 21806 | 2 osob] Wchodzimy do baru!
	[KLIENT 21795 | 2 osob] Wszystkie stoliki sa zarezerwowane. Wychodzimy!
	[KLIENT 21792 | 2 osob] Wszystkie stoliki sa zarezerwowane. Wychodzimy!
	[KLIENT 21802 | 2 osob] Wszystkie stoliki sa zarezerwowane. Wychodzimy!
	[KLIENT 21807 | 2 osob] Wchodzimy do baru!
	[KLIENT 19640 | 3 osob] Wszystkie stoliki sa zarezerwowane. Wychodzimy!
	[KLIENT 19651 | 3 osob] Wszystkie stoliki sa zarezerwowane. Wychodzimy!
	[KLIENT 19662 | 3 osob] Wchodzimy do baru!
	[KLIENT 21804 | 2 osob] Wszystkie stoliki sa zarezerwowane. Wychodzimy!
	[KLIENT 19650 | 3 osob] Wszystkie stoliki sa zarezerwowane. Wychodzimy!
	[KLIENT 19665 | 3 osob] Wchodzimy do baru!
	[KLIENT 19661 | 3 osob] Wszystkie stoliki sa zarezerwowane. Wychodzimy!
	[KLIENT 19644 | 3 osob] Wszystkie stoliki sa zarezerwowane. Wychodzimy!
	[KLIENT 19655 | 3 osob] Wszystkie stoliki sa zarezerwowane. Wychodzimy!
	[KLIENT 21809 | 2 osob] Wchodzimy do baru!
	[KLIENT 21814 | 2 osob] Wchodzimy do baru!
	[KLIENT 21813 | 2 osob] Wchodzimy do baru!
	[KLIENT 19670 | 3 osob] Wchodzimy do baru!
	...
	```
	Podwajamy stoliki trzy osobowe. Logi w tym momencie:
	```
	[KLIENT 21836 | 2 osob] Wchodzimy do baru!
	[PRACOWNIK] Podwojono stoliki 3 osobowe: bylo: 5 teraz = 10 allTables = 25
	[KLIENT 21839 | 2 osob] Wchodzimy do baru!
	[KLIENT 19697 | 3 osob] Wchodzimy do baru!
	[KLIENT 19683 | 3 osob] Wchodzimy do baru!
	[KLIENT 19683 | 3 osob] Szukamy stolika 3 osobowego
	[KLIENT 19691 | 3 osob] Wchodzimy do baru!
	[KLIENT 19701 | 3 osob] Wchodzimy do baru!
	[KLIENT 19691 | 3 osob] Szukamy stolika 3 osobowego
	[KLIENT 21834 | 2 osob] Wchodzimy do baru!
	[KLIENT 21839 | 2 osob] Szukamy stolika 2 osobowego
	[KLIENT 19697 | 3 osob] Szukamy stolika 3 osobowego
	[KLIENT 19683 | Osoba 2] Zamawiam zestaw, zupa + danie glowne (30zl)!
	[KLIENT 19683 | Osoba 3] Zamawiam zestaw, zupa + danie glowne (30zl)!
	[KLIENT 19691 | Osoba 1] Zamawiam zestaw, zupa + danie glowne (30zl)!
	[KLIENT 19691 | Osoba 2] Zamawiam danie glowne (27zl)!
	[KLIENT 19691 | Osoba 3] Zamawiam zestaw, zupa + danie glowne (30zl)!
	[KLIENT 21824 | 2 osob] Szukamy stolika 2 osobowego
	[KASJER] Klient 19691 zaplacil 87 zl za zamowienie
	[KLIENT 19668 | 3 osob] Szukamy stolika 3 osobowego
	[KLIENT 19671 | 3 osob] Szukamy stolika 3 osobowego
	[KLIENT 19674 | 3 osob] Szukamy stolika 3 osobowego
	```
	Symulacja wraca do "normalności"
	Test zdany!
2. Wysłanie sygnału pożaru
	Oczekiwany rezultat: Najpierw bar opuszczają klienci. Gdy już nie ma klientów opuszczają bar kasjer i pracownik (kolejność tutaj nie ma znaczenia). W systemie nie zostaną procesy zombie ani IPC.
	```
	[MAIN] POZAR! Ewakuuje klientow
	[MAIN] Klienci ewakuowani
	[PRACOWNIK] Klienci ewakuowani, uciekam tez!
	[KASJER] Klienci ewakuowani, zamykam kase i uciekam
	[KASJER] Dochod: 1476
	[MAIN] Koniec symulacji
	```
	W poszukiwaniu procesów zombie:
	```
	student@LAPTOP-DKTH61LA:~/Bar_Mleczny_ProjektSO$ ps -aux | grep Z
	USER         PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
	student    26701  0.0  0.0   6564  2176 pts/5    S+   16:14   0:00 grep --color=auto Z
	```
	W systemie nie ma żadnych systemów zombie. Sprawdźmy jeszcze IPC:
	```
	student@LAPTOP-DKTH61LA:~/Bar_Mleczny_ProjektSO$ ipcs

	------ Kolejki komunikatów ---
	klucz      id_msq     właściciel uprawn.    bajtów      komunikatów
	
	------ Segmenty pamięci dzielonej ----
	klucz      id_shm     właściciel uprawn.    bajtów    podłączeń stan

	------ Tablice semaforów -------
	klucz      id_sem     właściciel uprawn.    lsem
	```
	Test zdany!
3. Czy klienci opuszczają bar z powodu frustracji.
	Oczekiwany rezultat: Podczas algorytmu szukania stoliku istnieje możliwość, że klient po kilku próbach wciąż nie znajdzie stolika. Sprawdźmy czy po kilku próbach klient zrezygnuje i wyjdzie z baru. Do tego testu ustawie czas jedzenia na `sleep(100)` i zmienię `if(attempts  >=  10)` na `if(attempts  >=  2)`. Jeśli po dwóch próbach klient nie znajdzie miejsca powinien wyjść z baru.
	Logi:
	```
	[KLIENT 28150 | 2 osob] Zbyt dlugo szukamy stolika. Wychodzimy!
	[KLIENT 28153 | 3 osob] Zbyt dlugo szukamy stolika. Wychodzimy!
	[KLIENT 28185 | 1 osob] Zbyt dlugo szukamy stolika. Wychodzimy!
	[KLIENT 28189 | 1 osob] Zbyt dlugo szukamy stolika. Wychodzimy!
	[KLIENT 28177 | 1 osob] Zbyt dlugo szukamy stolika. Wychodzimy!
	[KLIENT 28196 | 1 osob] Zbyt dlugo szukamy stolika. Wychodzimy!
	[KLIENT 28171 | 2 osob] Zbyt dlugo szukamy stolika. Wychodzimy!
	[KLIENT 28174 | 3 osob] Zbyt dlugo szukamy stolika. Wychodzimy!
	[KLIENT 28175 | 2 osob] Zbyt dlugo szukamy stolika. Wychodzimy!
	```
	Znaleźli się klienci sfrustrowani. Opuścili bar. Test zdany.
4. Czy klienci dosiadają się do grup równolicznych
	W tym przypadku przeanalizujemy logi jednego ze stolików po symulacji
	Logi:
	```
	[KLIENT 45379 | 1 osob] Siadamy przy stoliku 4 osobowym (id: 16). Siedzi przy nim 0 klientow
	[KLIENT 45386 | 1 osob] Siadamy przy stoliku 4 osobowym (id: 16). Siedzi przy nim 1 klientow
	[KLIENT 45396 | 1 osob] Siadamy przy stoliku 4 osobowym (id: 16). Siedzi przy nim 2 klientow
	[KLIENT 45398 | 1 osob] Siadamy przy stoliku 4 osobowym (id: 16). Siedzi przy nim 3 klientow
	[KLIENT 45419 | 1 osob] Siadamy przy stoliku 4 osobowym (id: 16). Siedzi przy nim 3 klientow
	```
	Jak możemy zauważyć licznik ile klientów siedzi się zwiększał o jeden co klienta [grupa 1 os] co potwierdza, że zasada równoliczności grup przy stołach jest spełniona.
5. Czy grupa klientów przed opuszczeniem baru czeka aż każdy wątek skończy jeść?
	Do tego testu dodam dodatkowego printa "Skończyłem jeść" w funkcji wątku. Na podstawie logów zaobserwujemy czy faktycznie grupa czeka na siebie.
	Logi:
	```
	[KLIENT 56636 | 3 osob] Wchodzimy do baru!
	[KLIENT 56636 | 3 osob] Szukamy stolika 3 osobowego
	[KLIENT 56636 | Osoba 2] Zamawiam zestaw, zupa + danie glowne (30zl)!
	[KLIENT 56636 | Osoba 1] Zamawiam zestaw, zupa + danie glowne (30zl)!
	[KLIENT 56636 | Osoba 3] Zamawiam zestaw, zupa + danie glowne (30zl)!
	[KASJER] Klient 56636 zaplacil 90 zl za zamowienie
	[KLIENT 56636 | 3 osob] Zaplacilismy za jedzenie 90 zl
	[PRACOWNIK] Wydaje zamowienie dla 56636
	[KLIENT 56636 | 3 osob] Odebralismy jedzenie od pracownika
	[KLIENT 56636 | 3 osob] Siadamy przy stoliku 3 osobowym (id: 14). Siedzi przy nim 0 klientow
	[KLIENT 56636 | Osoba 2] Jem (7 sek)!
	[KLIENT 56636 | Osoba 1] Jem (4 sek)!
	[KLIENT 56636 | Osoba 3] Jem (4 sek)!
	[KLIENT 56636 | Osoba 1] Skonczylem jesc!
	[KLIENT 56636 | Osoba 3] Skonczylem jesc!
	[KLIENT 56636 | Osoba 2] Skonczylem jesc!
	[KLIENT 56636 | 3 osob] Odnosimy naczynia i opuszczamy bar
	```
	Na podstawie logów dla jednego klienta o PID: 56636 możemy zauważyć że osoby w grupie (wątki) miały różny czas jedzenia. Osoba nr 2 zaczeła jeść jako pierwsza, skończyła jako ostatnia. Dopiero gdy cała trójka skończyła jeść odnieśli naczynia i opuścili bar.
	Test zdany!

---
## 7. Linki do kodu dla funkcji systemowych
1. Tworzenie procesów:
- fork() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/main.c#L154-L167)
- execl() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/main.c#L157)
- exit() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/main.c#L166)
2. Tworzenie wątków:
- pthread_create() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/main.c#L188-L204) 
- pthread_join() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/main.c#L228-L230)
3. Pamięć dzielona:
- ftok() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/ipc.c#L25-L29)
- shmget() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/ipc.c#L37-L46)
- shmat() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/ipc.c#L48-L52)
- shmdt() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/ipc.c#L218-L221)
- shmctl() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/ipc.c#L227-L231)
4. Semafory:
- semget() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/ipc.c#L111-L120)
- semctl() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/ipc.c#L233-L237)
- semop() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/ipc.c#L262-L271)
5. Kolejki komunikatów:
- msgget() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/ipc.c#L151-L164)
- msgsnd() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/ipc.c#L365-L367) 
- msgrcv() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/ipc.c#L381-L391)
- msgctl() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/ipc.c#L239-L247)
6. Obsługa plików:
- open() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/ipc.c#L396-L399)
- close() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/ipc.c#L404)
- write() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/ipc.c#L444)
7. Sygnały:
- kill() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/main.c#L238)
- signal() - [link](https://github.com/pturzynski/Bar_Mleczny_ProjektSO/blob/47659097d204b6e5f3b53d0a80407ed866c7d40e/src/main.c#L248)

