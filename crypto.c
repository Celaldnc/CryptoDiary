#include <stdio.h>      // Girdi/çıktı işlemleri
#include <stdlib.h>     // Temel C fonksiyonları (atoi, malloc, vs.)
#include <string.h>     // String işlemleri
#include <time.h>       // Zaman ve tarih işlemleri
#include <sys/stat.h>   // Dosya ve klasör bilgisi için
#include <unistd.h>     // mkdir, access gibi POSIX fonksiyonlar

// Maksimum uzunluk tanımlamaları
#define MAX_PASSWORD_LENGTH 50       // Maksimum şifre uzunluğu
#define MAX_TITLE_LENGTH 100         // Başlık uzunluğu
#define MAX_CONTENT_LENGTH 1000      // Günlük içerik uzunluğu
#define MAX_PATH_LENGTH 260          // Dosya yolu için maksimum uzunluk

// Global değişken: Günlük klasörü yolu saklanır
char DIARY_FOLDER[MAX_PATH_LENGTH];

// Günlük verilerini tutan yapı
typedef struct {
    char title[MAX_TITLE_LENGTH];       // Başlık
    char content[MAX_CONTENT_LENGTH];   // İçerik (şifrelenmiş halde kaydedilir)
    char password[MAX_PASSWORD_LENGTH]; // Şifre (metin formatında saklanır, daha sonra sayıya dönüştürülür)
    char date[20];                      // Oluşturulma tarihi (gg-aa-yyyy ss:dd)
    char recovery_question[100]; // Gizli soru
    char recovery_answer[100];   // Gizli cevabın düz metni
} DiaryEntry;

// Verilen yol klasör mü diye kontrol eder
int directoryExists(const char* path) {
    struct stat info;
    if (stat(path, &info) != 0) return 0; // Klasör yoksa 0 döner
    return S_ISDIR(info.st_mode);         // Klasör varsa 1 döner
}

// Belirtilen yolda klasör oluşturur
int createDirectory(const char* path) {
    return mkdir(path, 0755) == 0;  // Klasörü 755 izniyle oluştur (Linux/UNIX)
}

// Kullanıcının masaüstü yolunu belirler ve günlük klasörü oluşturmak için kullanılır
void getDesktopPath() {
    const char* homeDir = getenv("HOME");  // Kullanıcının home dizini alınır
    if (homeDir == NULL) {
        fprintf(stderr, "Could not get HOME directory\n");
        exit(1);
    }
    // Masaüstü altına "CryptoDiary" klasörü tanımlanır
    sprintf(DIARY_FOLDER, "%s/Desktop/CryptoDiary/", homeDir);
}

// Caesar Cipher algoritması: Metni verilen anahtar kadar kaydırarak şifreler
void caesarEncrypt(char* text, int key) {
    for (int i = 0; text[i] != '\0'; i++) {
        unsigned char c = text[i];
        // Küçük harfler için kaydırma
        if (c >= 'a' && c <= 'z') text[i] = ((c - 'a' + key) % 26) + 'a';
        // Büyük harfler için kaydırma
        else if (c >= 'A' && c <= 'Z') text[i] = ((c - 'A' + key) % 26) + 'A';
        // Rakamlar için kaydırma
        else if (c >= '0' && c <= '9') text[i] = ((c - '0' + key) % 10) + '0';
        // Diğer karakterler aynen bırakılır (isteğe bağlı olarak değiştirebilirsin)
        // TÜRKÇE karakter gibi yüksek baytlı UTF-8 karakterleri **hiç dokunma**
        else if (c < 128) text[i] = c; 
    }

}

// Caesar Cipher çözümü: Metni verilen anahtar kadar geri kaydırır (şifreyi çözer)
void caesarDecrypt(char* text, int key) {
    for (int i = 0; text[i] != '\0'; i++) {
       unsigned char c = text[i];
        // Küçük harfler için çözüm
        if (c >= 'a' && c <= 'z') text[i] = ((c - 'a' - key + 26) % 26) + 'a';
        // Büyük harfler için çözüm
        else if (c >= 'A' && c <= 'Z') text[i] = ((c - 'A' - key + 26) % 26) + 'A';
        // Rakamlar için çözüm
        else if (c >= '0' && c <= '9') text[i] = ((c - '0' - key + 10) % 10) + '0';
        // TÜRKÇE karakter gibi yüksek baytlı UTF-8 karakterleri **hiç dokunma**
        else if (c < 128) text[i] = c; 
    }
}

// Kullanıcıdan yalnızca pozitif tam sayı alınmasını sağlar
int getPositiveKeyFromUser() {
    char buffer[100];  // Geçici karakter dizisi
    int key;
    while (1) {
        printf("Password (1-25 number only): ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Satır sonu karakterini kaldır

        // Girilen değerin sadece rakamlardan oluşup oluşmadığını kontrol et
        int isValid = 1;
        for (int i = 0; buffer[i] != '\0'; i++) {
            if (buffer[i] < '0' || buffer[i] > '9') {
                isValid = 0;
                break;
            }
        }

        // Geçerli ve boş olmayan pozitif sayı ise, integer'a dönüştürüp döndür
        if (isValid && strlen(buffer) > 0) {
            key = atoi(buffer);
            if (key >= 1 && key <= 25) return key;
        }
        // Geçersiz giriş yapılırsa uyarı verilir
        printf("Invalid input! Please enter a positive number.\n");
    }
}

// Güncel tarih ve saat bilgisini "gg-aa-yyyy ss:dd" formatında alır
void getCurrentDate(char* dateStr) {
    time_t t = time(NULL);           // Şu anki zamanı al
    struct tm *tm = localtime(&t);   // Yerel zamana çevir
    strftime(dateStr, 20, "%d-%m-%Y %H:%M", tm); // Tarihi string formatına dönüştür
}
// Kullanıcıdan başlık, içerik ve şifre alarak şifrelenmiş günlük dosyası oluşturur
void createEntry() {
    DiaryEntry entry;
    char filename[MAX_TITLE_LENGTH + 100];

    printf("\n=== New Diary Entry ===\n");

    // Başlık al
    printf("Title: ");
    fgets(entry.title, MAX_TITLE_LENGTH, stdin);
    entry.title[strcspn(entry.title, "\n")] = 0; // Satır sonunu sil

    // İçerik al
    printf("Content (enter '.' on a new line to finish):\n");
    entry.content[0] = '\0'; // içerik sıfırla
    char line[100];

    // İçerik satır satır alınır, kullanıcı "." yazana kadar devam eder
    while (1) {
        fgets(line, sizeof(line), stdin);
        if (strcmp(line, ".\n") == 0 || strcmp(line, ".\r\n") == 0) break;
        if (strlen(entry.content) + strlen(line) < MAX_CONTENT_LENGTH) {
            strcat(entry.content, line);
        } else {
            printf("Content too long! Saving...\n");
            break;
        }
    }

    printf("Recovery Question (e.g., Your pet's name?): ");
    fgets(entry.recovery_question, sizeof(entry.recovery_question), stdin);
    entry.recovery_question[strcspn(entry.recovery_question, "\n")] = 0;

    printf("Recovery Answer: ");
    fgets(entry.recovery_answer, sizeof(entry.recovery_answer), stdin);
    entry.recovery_answer[strcspn(entry.recovery_answer, "\n")] = 0;


    // Şifre (anahtar) pozitif sayı olarak alınır
    int key = getPositiveKeyFromUser();
    sprintf(entry.password, "%d", key); // string olarak kaydedilir

    // İçerik şifrelenir
    caesarEncrypt(entry.content, key);

    // Tarih bilgisi eklenir
    getCurrentDate(entry.date);

    // Dosya ismi oluşturulur (başlığa göre)
    sprintf(filename, "%s%s.txt", DIARY_FOLDER, entry.title);
    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("Could not create file!\n");
        return;
    }

    // Günlük dosyaya yazılır
    fwrite(&entry, sizeof(DiaryEntry), 1, file);
    fclose(file);

    printf("Diary entry successfully saved!\n");
}

// Günlüğü şifrelenmiş haliyle gösterir, doğru şifre girilirse çözüp okunabilir hale getirir
void readEntry(const char* filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("File not found!\n");
        return;
    }

    DiaryEntry entry;
    fread(&entry, sizeof(DiaryEntry), 1, file);
    fclose(file);

    // Şifreli halini ekranda göster
    printf("\nEncrypted content:\n%s\n", entry.content);

    // Kullanıcıdan şifre al
    int key = getPositiveKeyFromUser();
    int trueKey = atoi(entry.password); // dosyada kayıtlı olan şifre

    // Şifre doğrulama
    if (key != trueKey) {
        printf("Incorrect password!\n");
        return;
    }

    // İçerik deşifre edilir
    caesarDecrypt(entry.content, key);

    // Günlük gösterilir
    printf("\n=== %s (%s) ===\n", entry.title, entry.date);
    printf("%s\n", entry.content);
}

// Günlük silmek için şifre doğrulaması yapılır
void deleteEntry(const char* filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("File not found!\n");
        return;
    }

    DiaryEntry entry;
    fread(&entry, sizeof(DiaryEntry), 1, file);
    fclose(file);

    // Şifre al ve kontrol et
    int key = getPositiveKeyFromUser();
    int trueKey = atoi(entry.password);

    if (key != trueKey) {
        printf("Incorrect password!\n");
        return;
    }

    // Silme işlemi
    if (remove(filename) == 0)
        printf("Diary entry successfully deleted!\n");
    else
        printf("Could not delete diary entry.\n");
}

// Tüm kayıtlı günlük başlıklarını listeler
void listEntries() {
    FILE *fp;
    char command[300];
    sprintf(command, "ls %s*.txt 2>/dev/null", DIARY_FOLDER);

    printf("\n=== Current Diary Entries ===\n");
    fp = popen(command, "r");
    if (!fp) {
        printf("Could not execute list command.\n");
        return;
    }

    char filename[MAX_TITLE_LENGTH + 100];
    int count = 0;

    while (fgets(filename, sizeof(filename), fp)) {
        filename[strcspn(filename, "\n")] = 0;
        char *baseFileName = strrchr(filename, '/');
        baseFileName = baseFileName ? baseFileName + 1 : filename;

        char *ext = strstr(baseFileName, ".txt");
        if (ext) *ext = '\0';

        printf("%s\n", baseFileName);
        count++;
    }

    pclose(fp);
    if (count == 0) printf("No diary entries found!\n");
}

// Kullanıcıdan günlük ismi alır ve okuma işlemini başlatır
void viewEntries() {
    listEntries(); // Günlükleri göster

    // Hangi günlük okunacak?
    printf("Enter the name of the file you want to read: ");
    char filename[MAX_TITLE_LENGTH];
    fgets(filename, MAX_TITLE_LENGTH, stdin);
    filename[strcspn(filename, "\n")] = 0;

    char fullFilename[MAX_TITLE_LENGTH + 100];
    sprintf(fullFilename, "%s%s.txt", DIARY_FOLDER, filename);

    readEntry(fullFilename);
}
void recoverPassword() {
    listEntries();

    printf("Entddedddr the name of the file you want to recover password for: ");
    char filename[MAX_TITLE_LENGTH];
    fgets(filename, MAX_TITLE_LENGTH, stdin);
    filename[strcspn(filename, "\n")] = 0;

    char fullFilename[MAX_TITLE_LENGTH + 200];
    sprintf(fullFilename, "%s%s.txt", DIARY_FOLDER, filename);

    FILE *file = fopen(fullFilename, "rb");
    if (!file) {
        printf("File not found!\n");
        return;
    }

    DiaryEntry entry;
    fread(&entry, sizeof(DiaryEntry), 1, file);
    fclose(file);

    printf("\nRecovery Question: %s\n", entry.recovery_question);
    printf("Answer: ");

    char answer[100];
    fgets(answer, sizeof(answer), stdin);
    answer[strcspn(answer, "\n")] = 0;

    if (strcmp(answer, entry.recovery_answer) == 0) {
        printf("✅ Correct! Your password is: %s\n", entry.password);
    } else {
        printf("❌ Incorrect answer. Password recovery failed.\n");
    }
}


// Ana menü: kullanıcıya seçimler sunar
int main() {
    int choice;

    getDesktopPath(); // Günlük klasörü tanımla
    if (!directoryExists(DIARY_FOLDER)) {
        printf("Creating CryptoDiary folder...\n");
        if (!createDirectory(DIARY_FOLDER)) {
            printf("ERROR: Could not create folder! Exiting...\n");
            return 1;
        }
        printf("Folder successfully created: %s\n", DIARY_FOLDER);
    }

    // Menü döngüsü
    while (1) {
        printf("\n===== CryptoDiary =====\n");
        printf("Folder: %s\n", DIARY_FOLDER);
        printf("1. Create new diary entry\n");
        printf("2. View diary entries\n");
        printf("3. Delete diary entry\n");
        printf("4. Recover forgotten password\n");
        printf("5. Exit\n");
        printf("Your choice: ");
        scanf("%d", &choice);
        getchar(); // satır sonu temizle

        switch (choice) {
            case 1:
                createEntry();
                break;
            case 2:
                viewEntries();
                break;
            case 3: {
                listEntries();
                printf("Enter the name of the file you want to delete: ");
                char filename[MAX_TITLE_LENGTH];
                fgets(filename, MAX_TITLE_LENGTH, stdin);
                filename[strcspn(filename, "\n")] = 0;

                char fullFilename[MAX_TITLE_LENGTH + 100];
                sprintf(fullFilename, "%s%s.txt", DIARY_FOLDER, filename);

                deleteEntry(fullFilename);
                break;
            }
            case 4:
                recoverPassword();
                break;
            case 5:
                printf("Exiting CryptoDiary...\n");
                return 0;
            default:
                printf("Invalid choice! Try again.\n");

        }
    }

    return 0;
}