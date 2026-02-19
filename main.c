#pragma warning(disable: 4996)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <curl/curl.h>

#define PI 3.14159265358979323846
#define MAX_ARALIK 2000
#define INLIER_ESIK 0.01
#define MIN_INLIER_SAYISI 8

typedef struct
{
    double x; double y;
} Nokta;

typedef struct
{
    double egim, b_sabiti;
    int inliers_sayisi;
    double minX, maxX;
    double minY, maxY;
    int indexler[2000];
} Dogru;

typedef struct
{
    double x, y;
    int i, j;
    double aciDeg;
} Kesisim;

Nokta* noktalar;
int nokta_sayisi = 0;

Dogru* dogrular = NULL;
int dogru_sayisi = 0;

Kesisim* kesisimler = NULL;
int kesisim_sayisi = 0;


Nokta kutupsaldanKartezyene(double r, double theta)
{
    Nokta n;
    n.x = r * cos(theta);
    n.y = r * sin(theta);
    return n;
}

int benzerDogrulariFiltreleme(Dogru y, Dogru m)
{
    if (fabs(y.egim - m.egim) < 0.4 && fabs(y.b_sabiti - m.b_sabiti) < 0.4)
        return 1;
    return 0;
}

int dogruKesisimleri(const Dogru a, const Dogru b, Nokta* P, double* aciDeg)
{
    double tEgim = (a.egim - b.egim);
    if (fabs(tEgim) < 1e-9) return 0;

    double x = (b.b_sabiti - a.b_sabiti) / tEgim;
    double y = a.egim * x + a.b_sabiti;

    double tolerans = 1e-6;
    double aMinx = fmin(a.minX, a.maxX) - tolerans;
    double aMaxx = fmax(a.minX, a.maxX) + tolerans;

    double aMiny = fmin(a.minY, a.maxY) - tolerans;
    double aMaxy = fmax(a.minY, a.maxY) + tolerans;

    double bMinx = fmin(b.minX, b.maxX) - tolerans;
    double bMaxx = fmax(b.minX, b.maxX) + tolerans;

    double bMiny = fmin(b.minY, b.maxY) - tolerans;
    double bMaxy = fmax(b.minY, b.maxY) + tolerans;

    int a_da = (x >= aMinx && x <= aMaxx && y >= aMiny && y <= aMaxy);
    int b_de = (x >= bMinx && x <= bMaxx && y >= bMiny && y <= bMaxy);
    if (!a_da || !b_de) return 0;

    double t = fabs((b.egim - a.egim) / (1.0 + a.egim * b.egim));
    double aci = atan(t) * 180.0 / PI;

    P->x = x;
    P->y = y;
    *aciDeg = aci;
    return 1;
}

int linkAcma(const char* link, const char* cikanDosya) {
    CURL* c_link;
    FILE* okunan_link;
    CURLcode donut;

    c_link = curl_easy_init();   //curl oturumunu başlatma
    if (c_link) {
        okunan_link = fopen(cikanDosya, "wb");  //linki yeni dosya olarak açma
        if (okunan_link == NULL) {
            printf("Gecici dosya olusturulamadi!\n");
            curl_easy_cleanup(c_link);
            return 0;
        }

        curl_easy_setopt(c_link, CURLOPT_URL, link);  //indirilecek url adresi 
        curl_easy_setopt(c_link, CURLOPT_WRITEDATA, okunan_link);  //indirilen dosyanın yazılacağı dosya
        curl_easy_setopt(c_link, CURLOPT_FAILONERROR, 1L); //404 hatası için


        donut = curl_easy_perform(c_link); //içeriği yeni dosyaya yazma


        curl_easy_cleanup(c_link);
        fclose(okunan_link);

        if (donut != CURLE_OK) {
            printf("Dosya indirilemedi: %s\n", curl_easy_strerror(donut));
            return 0;
        }

        return 1;
    }
    return 0;
}

static void lejantCizdirme(ALLEGRO_FONT* lejantFont, int ekranW, int ekranH) {
    const float kenarBoslugu = 30.f;
    const float lejantGenisligi = 320.f;

    const float sol = ekranW - lejantGenisligi - kenarBoslugu;
    const float ust = kenarBoslugu;
    const float sag = ekranW - kenarBoslugu;
    const float alt = ekranH - kenarBoslugu;

    // Renkler
    ALLEGRO_COLOR lejantArkaplani = al_map_rgb(250, 250, 250);
    ALLEGRO_COLOR lejantCerceve = al_map_rgb(210, 210, 210);
    ALLEGRO_COLOR yazilar = al_map_rgb(0, 0, 0);
    ALLEGRO_COLOR noktalar_renki = al_map_rgb(100, 100, 100);
    ALLEGRO_COLOR robot_renki = al_map_rgb(255, 0, 0);
    ALLEGRO_COLOR dogru_renki = al_map_rgb(0, 150, 0);
    ALLEGRO_COLOR inlier_renki = al_map_rgb(255, 128, 0);
    ALLEGRO_COLOR kesisim_renki = al_map_rgb(65, 105, 225);
    ALLEGRO_COLOR mesafe_renki = al_map_rgb(255, 0, 0);

    al_draw_filled_rectangle(sol, ust, sag, alt, lejantArkaplani);  //lejantı çizer 
    al_draw_rectangle(sol, ust, sag, alt, lejantCerceve, 2.f); //2 piksel kalınlıkla çerçeve çizer

    const float yazininYuksekligi = (float)al_get_font_line_height(lejantFont);
    float solKenarUzakligi = sol + 14.f;
    float ustKenarUzakligi = ust + 14.f;

    al_draw_text(lejantFont, yazilar, solKenarUzakligi, ustKenarUzakligi, 0, "Aciklama");
    ustKenarUzakligi += yazininYuksekligi + 8.f;

    al_draw_filled_circle(solKenarUzakligi + 8, ustKenarUzakligi + yazininYuksekligi * 0.5f, 4.f, noktalar_renki);
    al_draw_text(lejantFont, yazilar, solKenarUzakligi + 26.f, ustKenarUzakligi, 0, "Tum Noktalar");
    ustKenarUzakligi += yazininYuksekligi + 6.f;

    al_draw_filled_circle(solKenarUzakligi + 8, ustKenarUzakligi + yazininYuksekligi * 0.5f, 6.f, robot_renki);
    al_draw_text(lejantFont, yazilar, solKenarUzakligi + 26.f, ustKenarUzakligi, 0, "Robot");
    ustKenarUzakligi += yazininYuksekligi + 10.f;

    for (int i = 0; i < dogru_sayisi; ++i)  //Doğruları lejanta yazma
    {
        float cizgiDikey = ustKenarUzakligi + yazininYuksekligi * 0.5f;
        al_draw_line(solKenarUzakligi + 4.f, cizgiDikey, solKenarUzakligi + 24.f, cizgiDikey, dogru_renki, 4.f);
        al_draw_filled_circle(solKenarUzakligi + 10.f, cizgiDikey, 3.f, inlier_renki);

        double dx = dogrular[i].maxX - dogrular[i].minX;
        double dy = dogrular[i].maxY - dogrular[i].minY;

        double cizgiUzunlugu = sqrt(dx * dx + dy * dy);
        char buf[160];
        snprintf(buf, sizeof(buf), "d%d: %d nokta, ~ %.2fm", i + 1, dogrular[i].inliers_sayisi, cizgiUzunlugu);
        al_draw_text(lejantFont, yazilar, solKenarUzakligi + 32.f, ustKenarUzakligi, 0, buf);
        ustKenarUzakligi += yazininYuksekligi + 4.f;

        if (ustKenarUzakligi > alt - 8 * yazininYuksekligi) break; // taşmayı önle
    }
    ustKenarUzakligi += 4.f;

    float carpi_x = solKenarUzakligi + 12.f;
    float carpi_y = ustKenarUzakligi + yazininYuksekligi * 0.5f;

    al_draw_line(carpi_x - 7, carpi_y - 7, carpi_x + 7, carpi_y + 7, kesisim_renki, 3.f);
    al_draw_line(carpi_x - 7, carpi_y + 7, carpi_x + 7, carpi_y - 7, kesisim_renki, 3.f);

    al_draw_text(lejantFont, yazilar, solKenarUzakligi + 30.f, ustKenarUzakligi, 0, "60°+ Kesisim");
    ustKenarUzakligi += yazininYuksekligi + 6.f;

    //Kesikli çizgi
    float kesikliBaslg = solKenarUzakligi + 4.f;
    float kesikliBts = kesikliBaslg + 36.f;
    float cizgininKonum = ustKenarUzakligi + yazininYuksekligi * 0.5f;

    float cizgiUzunlugu = 7.f;
    float cizgiBoslugu = 5.f;

    for (float t = kesikliBaslg; t < kesikliBts; t += cizgiUzunlugu + cizgiBoslugu) {
        float e = fminf(t + cizgiUzunlugu, kesikliBts);
        al_draw_line(t, cizgininKonum, e, cizgininKonum, mesafe_renki, 2.5f);
    }
    al_draw_text(lejantFont, yazilar, solKenarUzakligi + 44.f, ustKenarUzakligi, 0, "Mesafe Cizgisi");
    ustKenarUzakligi += yazininYuksekligi + 8.f;

    al_draw_text(lejantFont, yazilar, solKenarUzakligi, ustKenarUzakligi, 0, "Kesisim Noktalari:");
    ustKenarUzakligi += yazininYuksekligi;

    int goster = (kesisim_sayisi < 6) ? kesisim_sayisi : 6;
    for (int k = 0; k < goster; ++k) {
        char kesisim_yazi[96];

        snprintf(kesisim_yazi, sizeof(kesisim_yazi), "I%d: (%.2f, %.2f)  %.0f°", k + 1, kesisimler[k].x, kesisimler[k].y, kesisimler[k].aciDeg);
        al_draw_text(lejantFont, yazilar, solKenarUzakligi + 8.f, ustKenarUzakligi, 0, kesisim_yazi);

        ustKenarUzakligi += yazininYuksekligi;

        if (ustKenarUzakligi > alt - yazininYuksekligi) break;
    }
}

int main()
{
    srand(time(NULL));

    //Kullanıcıdan alınacak .toml dosyasını alma ve açma
    char url[1500];
    FILE* dosya;
    const char* geciciDosyaAdi = "indirilen_lidar.toml";

    printf("Lutfen .toml linkini giriniz:  ");
    if (fgets(url, sizeof(url), stdin) == NULL)
    {
        printf("\nGecersiz link girildi...\n");
        return 1;
    }

    for (int i = 0; url[i] != '\0'; ++i)
    {
        if (url[i] == '\n')
        {
            url[i] = '\0';
            break;
        }
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);  //curl kütüphanesini başlatma

    printf(" ... \n");
    if (!linkAcma(url, geciciDosyaAdi))
    {
        printf("Girilen linkten .toml dosyasi indirelemedi.\n");
        curl_global_cleanup();
        return 1;
    }
    printf("Islem basarili... \n");

    dosya = fopen(geciciDosyaAdi, "r");
    if (dosya == NULL)
    {
        printf("Indirilen dosya (%s) acilamadi.\n", geciciDosyaAdi);
        curl_global_cleanup();
        return 1;
    }

    //Kullanıcıdan alınacak .toml dosyasını tek tek okuma    
    char satir[200];
    double* ranges = (double*)malloc(MAX_ARALIK * sizeof(double));

    if (!ranges)
    {
        fclose(dosya);
        curl_global_cleanup();
        return 1;
    }

    int sayac = 0;
    int rangesKontrol = 0;

    double range_min = 0.0;
    double range_max = 0.0;
    double angle_min = 0.0;
    double angle_max = 0.0;
    double angle_increment = 0.0;

    while (fgets(satir, sizeof(satir), dosya))
    {
        if (strstr(satir, "angle_min"))
        {
            sscanf(satir, "angle_min = %lf", &angle_min);
        }
        else if (strstr(satir, "angle_max"))
        {
            sscanf(satir, "angle_max = %lf", &angle_max);
        }
        else if (strstr(satir, "angle_increment"))
        {
            sscanf(satir, "angle_increment = %lf", &angle_increment);
        }
        else if (strstr(satir, "range_min"))
        {
            sscanf(satir, "range_min = %lf", &range_min);
        }
        else if (strstr(satir, "range_max"))
        {
            sscanf(satir, "range_max = %lf", &range_max);
        }
        else if (strstr(satir, "ranges = ["))
        {
            rangesKontrol = 1;
            char* baslangic = strchr(satir, '[');
            if (baslangic) baslangic++;

            char* token = strtok(baslangic, ",");
            while (token && sayac < MAX_ARALIK)
            {
                double deger = atof(token);
                if (deger != -1.0 && deger >= range_min && deger <= range_max)
                    ranges[sayac++] = deger;
                token = strtok(NULL, ",");
            }
        }

        else if (rangesKontrol)
        {
            if (strchr(satir, ']'))
            {
                rangesKontrol = 0;
                break;
            }

            char* token = strtok(satir, ",");
            while (token && sayac < MAX_ARALIK) {
                double deger = atof(token);
                if (deger != -1.0 && deger >= range_min && deger <= range_max)
                    ranges[sayac++] = deger;
                token = strtok(NULL, ",");
            }
        }
    }

    if (range_min == 0.0 && range_max == 0.0)
    {
        printf("Range min ve  Range max degerleri bulunamadi...\n");
        return 1;
    }

    fclose(dosya);

    noktalar = (Nokta*)malloc((size_t)sayac * sizeof(Nokta));

    dogrular = (Dogru*)malloc(50 * sizeof(Dogru));

    //Kutupsaldan Kartezyene dönüşüm
    for (int i = 0; i < sayac; i++)
    {
        double r = ranges[i];

        if (r != r || r<range_min || r>range_max) continue;

        double tAcisi = angle_min + i * angle_increment;

        Nokta n = kutupsaldanKartezyene(r, tAcisi);

        if (nokta_sayisi >= sayac) break;

        noktalar[nokta_sayisi++] = n;


        //printf("\nKutupsaldan kartezyene donusen noktalar: ");
        //printf("\nNokta %d: (x: %.2f, y: %.2f, r: %.2f, theta: %.2f rad)", i + 1, noktalar->x, noktalar->y , r, tAcisi);

    }

    free(ranges);

    int* kullanildi = (int*)calloc(nokta_sayisi, sizeof(int));

    //Doğruların tespiti (RANSAC)
    for (int i = 0; i < 20000; i++)
    {
        if (nokta_sayisi < 2) break;

        int in1 = rand() % nokta_sayisi;
        int in2 = rand() % nokta_sayisi;

        if (in1 == in2 || kullanildi[in1] || kullanildi[in2])
        {
            int deneme = 0;
            while ((in1 == in2 || kullanildi[in1] || kullanildi[in2]) && deneme < 100)
            {
                in1 = rand() % nokta_sayisi;
                in2 = rand() % nokta_sayisi;
                deneme++;
            }
            if (in1 == in2 || kullanildi[in1] || kullanildi[in2]) continue;
        }

        double x1 = noktalar[in1].x;
        double y1 = noktalar[in1].y;

        double x2 = noktalar[in2].x;
        double y2 = noktalar[in2].y;

        if (fabs(x2 - x1) < 1e-6) continue;

        double egim = (y2 - y1) / (x2 - x1);
        double b_sabiti = y1 - egim * x1;

        double minX = 9999, maxX = -9999, minY = 9999, maxY = -9999;

        int inlier = 0;
        int dogruInlier[2000]; //Doğrudaki inlier sayısını tutar
        int inlierSayaci = 0;

        for (int j = 0; j < nokta_sayisi; j++)
        {
            if (kullanildi[j]) continue;

            double x = noktalar[j].x, y = noktalar[j].y;
            double dikMesafe = fabs(egim * x - y + b_sabiti) / sqrt(egim * egim + 1);

            if (dikMesafe < INLIER_ESIK)
            {
                inlier++;
                dogruInlier[inlierSayaci++] = j;
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }

        if (inlier >= MIN_INLIER_SAYISI)
        {
            Dogru tDogru = { egim, b_sabiti, inlier, minX, maxX, minY, maxY };

            for (int k = 0; k < inlierSayaci; k++)
            {
                tDogru.indexler[k] = dogruInlier[k];
            }

            int benzerDogru = 0;
            for (int k = 0; k < dogru_sayisi; k++)
            {
                if (benzerDogrulariFiltreleme(tDogru, dogrular[k]))
                {
                    benzerDogru = 1;
                    break;
                }
            }

            if (benzerDogru) continue;
            if (dogru_sayisi >= 30) break;

            dogrular[dogru_sayisi++] = tDogru;

            for (int k = 0; k < inlierSayaci; k++)
            {
                kullanildi[dogruInlier[k]] = 1;
            }
        }
    }
    free(kullanildi);

    kesisimler = (Kesisim*)malloc(200 * sizeof(Kesisim));

    //Kesişim noktalarının bulunması ve açının hesabı
    for (int i = 0; i < dogru_sayisi; i++)
    {
        for (int j = i + 1; j < dogru_sayisi; j++)
        {
            Nokta P;
            double aci;
            if (dogruKesisimleri(dogrular[i], dogrular[j], &P, &aci))
            {
                if (aci >= 60)
                {
                    printf("Dogru%d ile Dogru%d %.2f derecelik aci yaparak kesisiyor.\n", i + 1, j + 1, aci);
                    printf("Kesisim Noktasi (x,y): (%.2f, %.2f)\n", P.x, P.y);

                    double mesafe = sqrt(P.x * P.x + P.y * P.y);
                    printf("Robot ile bu kesisim noktasi arasi mesafe: %.2f metre\n\n", mesafe);

                    if (kesisim_sayisi < 200)
                    {
                        Kesisim tKesisim;
                        tKesisim.x = P.x;
                        tKesisim.y = P.y;
                        tKesisim.i = i;
                        tKesisim.j = j;
                        tKesisim.aciDeg = aci;
                        kesisimler[kesisim_sayisi++] = tKesisim;
                    }
                }
            }
        }
    }

    //Görselleştirme (Allegro)
    al_init();
    al_init_primitives_addon();
    al_install_keyboard();
    al_init_font_addon();
    al_init_ttf_addon();

    const int eksenBoyutu = 1200;       //x ekseni
    const int genislikLejant = 320;
    const int bosluk = 20;
    const int ekranGenisligi = eksenBoyutu + genislikLejant + bosluk * 2;
    const int ekranYukseligi = 900;

    ALLEGRO_DISPLAY* ekran = al_create_display(1600, 900);
    ALLEGRO_EVENT_QUEUE* kuyruk = al_create_event_queue();
    al_register_event_source(kuyruk, al_get_keyboard_event_source());
    ALLEGRO_FONT* fontEksen = al_load_ttf_font("C:/Windows/Fonts/arial.ttf", 12, 0);
    ALLEGRO_FONT* fontLejant = al_load_ttf_font("C:/Windows/Fonts/arial.ttf", 20, 0);
    al_clear_to_color(al_map_rgb(0, 0, 0));


    al_clear_to_color(al_map_rgb(255, 255, 255)); // Beyaz arka plan

    const float solBosluk = bosluk;
    const float ustBosluk = bosluk;
    const float sagBosluk = bosluk + eksenBoyutu;
    const float altBosluk = bosluk + eksenBoyutu;

    float merkez_x = (ekranGenisligi - genislikLejant) / 2.0f;
    float merkez_y = ekranYukseligi / 2.0f;

    ALLEGRO_COLOR izgaraRenki = al_map_rgb(230, 230, 230);
    ALLEGRO_COLOR eksenRenki = al_map_rgb(100, 100, 100);
    ALLEGRO_COLOR yaziRenki = al_map_rgb(0, 0, 0);
    float olcek = 120.0f;

    for (int i = -5; i <= 5; i++) {
        if (i == 0) continue;
        float x = merkez_x + i * olcek;
        al_draw_line(x, ustBosluk, x, altBosluk, izgaraRenki, 1);
        al_draw_textf(fontEksen, yaziRenki, x, merkez_y + 5, ALLEGRO_ALIGN_CENTER, "%d", i);
    }

    for (int i = -4; i <= 4; i++) {
        if (i == 0) continue;
        float y = merkez_y - i * olcek;
        al_draw_line(solBosluk, y, sagBosluk, y, izgaraRenki, 1);
        al_draw_textf(fontEksen, yaziRenki, merkez_x - 5, y, ALLEGRO_ALIGN_RIGHT, "%d", i);
    }


    al_draw_line(merkez_x, ustBosluk, merkez_x, altBosluk, eksenRenki, 1.5);
    al_draw_line(solBosluk, merkez_y, sagBosluk, merkez_y, eksenRenki, 1.5);
    al_draw_text(fontEksen, yaziRenki, merkez_x + 6, merkez_y + 6, 0, "0");


    for (int i = 0; i < nokta_sayisi; i++)  //Noktalar
    {
        al_draw_filled_circle(merkez_x + noktalar[i].x * olcek, merkez_y - noktalar[i].y * olcek, 1.5, al_map_rgb(100, 100, 100));
    }

    for (int i = 0; i < dogru_sayisi; i++)  //Doğrular
    {
        double x1 = dogrular[i].minX;
        double x2 = dogrular[i].maxX;

        double y1 = dogrular[i].egim * x1 + dogrular[i].b_sabiti;
        double y2 = dogrular[i].egim * x2 + dogrular[i].b_sabiti;

        al_draw_line(merkez_x + x1 * olcek, merkez_y - y1 * olcek, merkez_x + x2 * olcek, merkez_y - y2 * olcek, al_map_rgb(152, 255, 152), 3.5);

        for (int j = 0; j < dogrular[i].inliers_sayisi; j++)
        {
            int idx = dogrular[i].indexler[j];
            if (idx < 0 || idx >= nokta_sayisi) continue;
            double sx = merkez_x + noktalar[idx].x * olcek;
            double sy = merkez_y - noktalar[idx].y * olcek;
            al_draw_filled_circle(sx, sy, 2.5, al_map_rgb(255, 128, 0));
        }

        double ortX = (x1 + x2) / 2.0;
        double ortY = (y1 + y2) / 2.0;
        double etiketX = merkez_x + ortX * olcek;
        double etiketY = merkez_y - ortY * olcek;

        char yazi[10];
        sprintf(yazi, "D%d", i + 1);

        ALLEGRO_COLOR yazi_renk = al_map_rgb(0, 0, 0);
        ALLEGRO_COLOR arka_renk = al_map_rgb(255, 255, 120);
        ALLEGRO_COLOR cerceve_renk = al_map_rgb(200, 200, 0);

        float genislik = al_get_text_width(fontEksen, yazi) + 6;
        float yukseklik = al_get_font_line_height(fontEksen);

        float sol = etiketX - genislik / 2;
        float ust = etiketY - yukseklik / 2;
        float sag = etiketX + genislik / 2;
        float alt = etiketY + yukseklik / 2;

        al_draw_filled_rectangle(sol, ust, sag, alt, arka_renk);
        al_draw_rectangle(sol, ust, sag, alt, cerceve_renk, 1.5);

        al_draw_text(fontEksen, yazi_renk, etiketX, etiketY - yukseklik / 2 + 2, ALLEGRO_ALIGN_CENTER, yazi);
    }

    for (int k = 0; k < kesisim_sayisi; k++) //Kesişimler
    {
        double sx = merkez_x + kesisimler[k].x * olcek; //kesişim x koordinatı
        double sy = merkez_y - kesisimler[k].y * olcek; //kesişim y koordinatı

        al_draw_line(sx - 5, sy - 5, sx + 5, sy + 5, al_map_rgb(65, 105, 225), 3.5);
        al_draw_line(sx - 5, sy + 5, sx + 5, sy - 5, al_map_rgb(65, 105, 225), 3.5);


        double mesafe = sqrt(kesisimler[k].x * kesisimler[k].x + kesisimler[k].y * kesisimler[k].y);

        ALLEGRO_COLOR info_color = al_map_rgb(0, 50, 150);
        al_draw_textf(fontEksen, info_color, sx + 8, sy - 10, 0, "(%.2fm, %.0f°)", mesafe, kesisimler[k].aciDeg);
    }
    //Robot
    al_draw_filled_circle(merkez_x, merkez_y, 5, al_map_rgb(255, 0, 0));

    //Etiketler
    if (kesisim_sayisi > 0) {
        double minMesafe = 9999.0;
        Kesisim enYakinKesisim;

        for (int k = 0; k < kesisim_sayisi; k++) {
            double mesafe = sqrt(kesisimler[k].x * kesisimler[k].x + kesisimler[k].y * kesisimler[k].y);
            if (mesafe < minMesafe) {
                minMesafe = mesafe;
                enYakinKesisim = kesisimler[k];
            }
        }

        double kx = merkez_x + enYakinKesisim.x * olcek;
        double ky = merkez_y - enYakinKesisim.y * olcek;

        al_draw_line(merkez_x, merkez_y, kx, ky, al_map_rgb(250, 128, 114), 3.5);

        al_draw_textf(fontEksen, yaziRenki, (merkez_x + kx) / 2 + 5, (merkez_y + ky) / 2, 0, "%.2fm", minMesafe);

        al_draw_textf(fontEksen, yaziRenki, kx + 5, ky - 15, ALLEGRO_ALIGN_CENTER, "%.0f\370", enYakinKesisim.aciDeg);
    }

    int ekranG = al_get_display_width(ekran);
    int ekranY = al_get_display_height(ekran);
    lejantCizdirme(fontLejant, ekranG, ekranY);

    al_flip_display();

    //Esc'ye basmadan kapanmasın
    bool calisiyor = true;
    while (calisiyor)
    {
        ALLEGRO_EVENT event;
        if (al_get_next_event(kuyruk, &event)) {
            if (event.type == ALLEGRO_EVENT_KEY_DOWN &&
                event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                calisiyor = false;
            }
        }
        al_rest(0.01);
    }

    al_destroy_event_queue(kuyruk);
    al_destroy_font(fontEksen);
    al_destroy_font(fontLejant);
    al_destroy_display(ekran);
    free(dogrular);
    free(noktalar);
    free(kesisimler);
    return 0;
}
