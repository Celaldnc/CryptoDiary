#include <stdio.h>      // Girdi/çıktı işlemleri
#include <stdlib.h>     // Temel C fonksiyonları (atoi, malloc, vs.)
#include <string.h>     // String işlemleri
#include <time.h>       // Zaman ve tarih işlemleri
#include <sys/stat.h>   // struct stat için gerekli

// Platform bağımsız başlık dosyaları
#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
    #define PATH_SEPARATOR "\\"
#else
    #include <unistd.h>
    #define PATH_SEPARATOR "/"
#endif

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
    #ifdef _WIN32
        return _mkdir(path) == 0;
    #else
        return mkdir(path, 0755) == 0;
    #endif
}

// Kullanıcının masaüstü yolunu belirler ve günlük klasörü oluşturmak için kullanılır
void getDesktopPath() {
    #ifdef _WIN32
        const char* homeDir = getenv("USERPROFILE");
    #else
        const char* homeDir = getenv("HOME");
    #endif

    if (homeDir == NULL) {
        fprintf(stderr, "Could not get home directory\n");
        exit(1);
    }
    // Masaüstü altına "CryptoDiary" klasörü tanımlanır
    sprintf(DIARY_FOLDER, "%s%sDesktop%sCryptoDiary%s", 
            homeDir, PATH_SEPARATOR, PATH_SEPARATOR, PATH_SEPARATOR);
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
        // TÜRKÇE karakter gibi yüksek baytlı UTF-8 karakterleri *hiç dokunma*
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
        // TÜRKÇE karakter gibi yüksek baytlı UTF-8 karakterleri *hiç dokunma*
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
    char title[MAX_TITLE_LENGTH];
    char content[MAX_CONTENT_LENGTH];
    char filename[MAX_TITLE_LENGTH + 100];

    printf("\n=== New Diary Entry ===\n");

    // Başlık al
    printf("Title: ");
    fgets(title, MAX_TITLE_LENGTH, stdin);
    title[strcspn(title, "\n")] = 0; // Satır sonunu sil

    // İçerik al
    printf("Content (enter '.' on a new line to finish):\n");
    content[0] = '\0';
    char line[100];
    while (1) {
        fgets(line, sizeof(line), stdin);
        if (strcmp(line, ".\n") == 0 || strcmp(line, ".\r\n") == 0) break;
        if (strlen(content) + strlen(line) < MAX_CONTENT_LENGTH) {
            strcat(content, line);
        } else {
            printf("Content too long! Saving...\n");
            break;
        }
    }

    // Şifre al
    int key = getPositiveKeyFromUser();

    // İçeriği şifrele
    caesarEncrypt(content, key);

    // Dosya ismi oluştur
    sprintf(filename, "%s%s.txt", DIARY_FOLDER, title);
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Could not create file!\n");
        return;
    }

    // Şifreli içeriği ve anahtarı dosyaya yaz
    fprintf(file, "%s", content);
    fclose(file);

    printf("Diary entry successfully saved!\n");
}

// Günlüğü şifrelenmiş haliyle gösterir, doğru şifre girilirse çözüp okunabilir hale getirir
void readEntry(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("File not found!\n");
        return;
    }

    int trueKey;
    char content[MAX_CONTENT_LENGTH];

    // İlk satırda anahtar var
    fscanf(file, "%d\n", &trueKey);
    // Kalanı şifreli içerik
    fgets(content, sizeof(content), file);
    fclose(file);

    // Şifre sor
    int key = getPositiveKeyFromUser();
    if (key != trueKey) {
        printf("Incorrect password!\n");
        return;
    }

    // Şifreyi çöz
    caesarDecrypt(content, key);

    printf("\n=== Diary Content ===\n");
    printf("%s\n", content);
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
    #ifdef _WIN32
        WIN32_FIND_DATA findFileData;
        HANDLE hFind;
        char searchPath[MAX_PATH_LENGTH];
        sprintf(searchPath, "%s*.txt", DIARY_FOLDER);
        
        printf("\n=== Current Diary Entries ===\n");
        hFind = FindFirstFile(searchPath, &findFileData);
        
        if (hFind == INVALID_HANDLE_VALUE) {
            printf("No diary entries found!\n");
            return;
        }
        
        int count = 0;
        do {
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                char* filename = findFileData.cFileName;
                char* ext = strstr(filename, ".txt");
                if (ext) *ext = '\0';
                printf("%s\n", filename);
                count++;
            }
        } while (FindNextFile(hFind, &findFileData));
        
        FindClose(hFind);
        if (count == 0) printf("No diary entries found!\n");
    #else
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
    #endif
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

    printf("Enter the name of the file you want to recover password for: ");
    char filename[MAX_TITLE_LENGTH];
    fgets(filename, MAX_TITLE_LENGTH, stdin);
    filename[strcspn(filename, "\n")] = 0;

    char fullFilename[MAX_TITLE_LENGTH + 100];
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