
#ifndef __COMPUTATION_H__
#define __COMPUTATION_H__



// před nastavená alokovací fce
void* my_alloc(size_t size);

// alokuje paměť pro iterací a nastavuje velikost pixelu a počet chunků
void computation_init(void);

// uvolňuje paměť pro uložení iterací a nastavuje udaje k výpočtům na nulu
void computation_cleanup(void);

// ověřuje jestli výpočet stále probíhá
bool is_computing(void);

// ověřuje jestli je výpočet dokončen
bool is_done(void);

// ověřuje jestli je výpočet přerušen
bool is_aborted(void);

// reaguje na přerušení výpočtu od uživatele a zadává na proměnou abort true
void abort_comp(void);

// resetuje proměnou abort a umožňuje znovu spustit výpočet 
void enable_comp(void);

// pošle nukleu potřebné data k výpočtům
bool set_compute(message *msg);

// spustí (pošle zprávu nukleu) výpočet pro jednotlivé části obrázku (chunků)
bool compute(message *msg);

// zapíše vypočítané iterace od nuklea do grid množiny pro další práci s nimi
void update_data(const msg_compute_data *compute_data);

// převezme rozměry z hlavní struktury
void get_grid_size(int *w,int *h);

// Otevře okno s příslušnými zadanými rozměry 
void gui_init(void);

// Uvolní paměť a přepíše obrázek na počátečný stav
void gui_cleanup(void);

// updatne obrázek barvami s případnými vypočtenými iteracemi pro daný pixel 
void update_image(int w,int h, unsigned char* img);

// fce přepíše obrázek s případným updatem v grafickém okně
void gui_refresh(void);

// načítá reálnou složku konstanty C
void load_cre(void);

// načítá imaginární složku konstanty C
void load_cim(void);

// Vytiskne na výstup uživateli informace o stávajících hodnotách konstanty C
void print_C(void);

// resetuje chunk_id
void reset_chunk(void);

// resetuje výpočet na počáteční hodnoty a lze tím spočítat nový obrázek
void restart_compute(void);

// výpočet počtu iterací po jednotlivý pixel
void compute_Julian_set(void);

// vrací barvu RGB pixelu podle iterací
void distribution_of_color(int k, int i, double* r, double* g, double* b);

// vypočítá Julian set na počítači
int computJulianSet(double re, double im, int k, double cre, double cim);

// fce na uložení obrázku ve formátu .ppm
void save_image(void);

// fce na animaci s pomocí plynulé změny konstanty C
void animation_image(void);

// fce zastavuje průběh animace
void stop_animation(void);

// Přebarvý grafické okno na černo
void gui_black_screen(void);

// Nastavý novou maximání honodtu v reálném intervalu
void load_max_re(void);

// Nastavý novou minimální honodtu v reálném intervalu
void loadminre(void);

// Vytiskne na výstup uživateli informace o stávajících hodnotách 
// maximální a minimální hodnoty reálného intervalu a velikost šířky jednoho pixelu 
void print_re_interval_values();

// spočítá novou hodnotu velikosti šířky nového pixelu
void comp_d_re(void);

// Nastavý novou maximání honodtu v imaginárním intervalu
void loadmin_im(void);

// Nastavý novou maximání honodtu v imaginárním intervalu
void loadmax_im(void);

// Vytiskne na výstup uživateli informace o stávajících hodnotách 
// maximální a minimální hodnoty imaginárního intervalu a velikost výšky jednoho pixelu 
void print_im_interval_values(void);

// spočítá novou hodnotu velikosti výšky nového pixelu
void comp_d_im(void);

// Nastavý novou maximání honodtu šířky obrázku
void change_wight_img(void);

// Nastavý novou maximání honodtu výšky obrázku
void change_height_img(void);

// Vytiskne na výstup stávající hodnotu o stavajících hodnotách 
// velikosti šířky a velikosti obrázku
void print_dimension_img(void);

// uvolní alokovanou paměť pro uložení vypočtených hodnot
void free_grid(void);

// Alokuje novou paměť pro grid a vypočítá z nových hodnot počet chunků 
void comp_new(void);

// zavře a otevře okno
void restart_grafic_windows(void);

// načte nový počet iterací, který bude vykonáván
void load_number_iteration(void);

// vzpíše na výstup stávající hodnotu počtu iterací 
void print_number_iteration(void);

// načte novou reálnou část o kterou se periodycky bude měnit 
// hodnota reálné části konstanty C v animaci ze standartního vstupu
void load_new_re_shift(void);

// načte novou imaginární část o kterou se periodycky bude měnit 
// hodnota imaginární části konstanty C v animaci ze standartního vstupu
void load_new_im_shift(void);

// vypiše informace o kterou se budou měnit jednotlivé části konstanty C
void print_animation_change_shift(void);

// přičte nastavenou hodnotu k réálné periodické změně části konstanty C 
void plus_shift_re(void);

// odečte nastavenou hodnotu k réálné periodické změně části konstanty C 
void minus_shift_re(void);

// přičte nastavenou hodnotu k imaginární periodické změně části konstanty C 
void plus_shift_im(void);

// odečte nastavenou hodnotu k imaginární periodické změně části konstanty C 
void minus_shift_im(void);

// vytiskne na vstup aktuální hodnotu změny jendotlivých částí konstanty C při zvyšování 
void print_animation_current_shirt(void);

// vytiskne na výstup všechna uložená data o výpočtech
void print_entered_data(void);

// vypíše všechny ovládací klíče na standartní výstup
void print_control_key(void);

// vrátí informaci o tom, jeslti byli již nastaveny data
bool is_set_data(void);

// Vytiskne na standartní výstup, který chunk byl vzpočítán
void print_info_chunk(void);

// Vrací hodnotu true, jeslti chceme pokračovat dál v animaci
void continue_animation(void);

// Změní animaci na opačnou stranu
void oposite_animation(void);

// načte reálnou složku konstanty C posunutí na hodnotu, kterou chce uživatel
void load_whole_new_shift_re(void);

// načte imaginární složku konstanty C posunutí na hodnotu, kterou chce uživatel
void load_whole_new_shift_im(void);

// vypíše základní data pro informaci uživatele o kontrolních klíčích při spuštění programu 
void print_start(void);

#endif
