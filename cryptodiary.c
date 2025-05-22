/*#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <direct.h> // Windows için _mkdir fonksiyonu
#include <windows.h> // Windows API için
#include <shlobj.h>  // SHGetFolderPath için
#define MAX_PASSWORD_LENGTH 50
#define MAX_TITLE_LENGTH 100
#define MAX_CONTENT_LENGTH 1000
#define ENCRYPTION_KEY 42
#define MAX_PATH_LENGTH 260

// Klasör yolu saklamak için global değişken
char DIARY_FOLDER[MAX_PATH_LENGTH];

// Klasörün var olup olmadığını kontrol eden fonksiyon
int directoryExists(const char* path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        return 0; // Klasör bulunamadı
    }zz
    return (info.st_mode & S_IFDIR); // Klasör mü?
}

// Klasör oluşturan fonksiyon
int createDirectory(const char* path) {
    #ifdef _WIN32
    return _mkdir(path) == 0;
    #else
    return mkdir(path, 0755) == 0;
    #endif
}

// Masaüstü yolunu alma fonksiyonu
void getDesktopPath() {
    char desktopPath[MAX_PATH_LENGTH];
    
    // Windows API ile Masaüstü klasörünün yolunu al
    SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath);
    
    // Diary klasörünün tam yolunu oluştur
    sprintf(DIARY_FOLDER, "%s\\CryptoDiary\\", desktopPath);
}

// Yapılar
typedef struct {
    char title[MAX_TITLE_LENGTH];
    char content[MAX_CONTENT_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    char date[20];
} DiaryEntry;

// Fonksiyon prototipleri
void createEntry();
void viewEntries();
void readEntry(const char* filename);
void deleteEntry(const char* filename);
void listEntries();
void encrypt(char* text, int key);
void decrypt(char* text, int key);
void getCurrentDate(char* dateStr);

int main() {
    int choice;
    
    // Create "CryptoDiary" folder on desktop
    getDesktopPath();
    
    // Create folder if it doesn't exist
    if (!directoryExists(DIARY_FOLDER)) {
        printf("Creating CryptoDiary folder...\n");
        if (!createDirectory(DIARY_FOLDER)) {
            printf("ERROR: Could not create folder! Exiting program.\n");
            return 1;
        }
        printf("Folder successfully created: %s\n", DIARY_FOLDER);
    }
    
    // Main menu
    while (1) {
        printf("\n===== CryptoDiary =====\n");
        printf("Folder: %s\n", DIARY_FOLDER);
        printf("1. Create new diary entry\n");
        printf("2. View diary entries\n");
        printf("3. Delete diary entry\n");
        printf("4. Exit\n");
        printf("Your choice: ");
        scanf("%d", &choice);
        getchar(); // Clear newline character
        
        switch (choice) {
            case 1:
                createEntry();
                break;
            case 2:
                viewEntries();
                break;
            case 3:
                listEntries();
                printf("Enter the name of the file you want to delete: ");
                char filename[MAX_TITLE_LENGTH];
                fgets(filename, MAX_TITLE_LENGTH, stdin);
                filename[strcspn(filename, "\n")] = 0; // Remove newline character
                
                // Add folder path and .txt extension to filename
                char fullFilename[MAX_TITLE_LENGTH + 100];
                sprintf(fullFilename, "%s%s.txt", DIARY_FOLDER, filename);
                
                deleteEntry(fullFilename);
                break;
            case 4:
                printf("Exiting CryptoDiary...\n");
                return 0;
            default:
                printf("Invalid choice! Please try again.\n");
        }
    }
    
    return 0;
}

// Create new diary entry
void createEntry() {
    DiaryEntry entry;
    char filename[MAX_TITLE_LENGTH + 4]; // Extra space for .txt extension
    
    printf("\n=== New Diary Entry ===\n");
    
    printf("Title: ");
    fgets(entry.title, MAX_TITLE_LENGTH, stdin);
    entry.title[strcspn(entry.title, "\n")] = 0; // Remove newline character
    
    printf("Content (enter '.' on a new line to finish):\n");
    
    char line[100];
    entry.content[0] = '\0';
    
    while (1) {
        fgets(line, sizeof(line), stdin);
        if (strcmp(line, ".\n") == 0 || strcmp(line, ".\r\n") == 0) {
            break;
        }
        
        if (strlen(entry.content) + strlen(line) < MAX_CONTENT_LENGTH) {
            strcat(entry.content, line);
        } else {
            printf("Content too long! Saving...\n");
            break;
        }
    }
    
    printf("Password: ");
    fgets(entry.password, MAX_PASSWORD_LENGTH, stdin);
    entry.password[strcspn(entry.password, "\n")] = 0; // Remove newline character
    
    // Encrypt password
    encrypt(entry.password, ENCRYPTION_KEY);
    
    // Encrypt content
    encrypt(entry.content, ENCRYPTION_KEY);
    
    // Get current date
    getCurrentDate(entry.date);
    
    // Create filename
    sprintf(filename, "%s%s.txt", DIARY_FOLDER, entry.title);
    
    // Write to file
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        printf("Could not create file!\n");
        return;
    }
    
    fwrite(&entry, sizeof(DiaryEntry), 1, file);
    fclose(file);
    
    printf("Diary entry successfully saved!\n");
}

// List and view diary entries
void viewEntries() {
    listEntries();
    
    printf("Enter the name of the file you want to read: ");
    char filename[MAX_TITLE_LENGTH];
    fgets(filename, MAX_TITLE_LENGTH, stdin);
    filename[strcspn(filename, "\n")] = 0; // Remove newline character
    
    // Add folder path and .txt extension to filename
    char fullFilename[MAX_TITLE_LENGTH + 100];
    sprintf(fullFilename, "%s%s.txt", DIARY_FOLDER, filename);
    
    readEntry(fullFilename);
}

// List all diary entries
void listEntries() {
    FILE *fp;
    char command[200];
    
    #ifdef _WIN32
    sprintf(command, "dir %s*.txt /b", DIARY_FOLDER);
    #else
    sprintf(command, "ls %s*.txt 2>/dev/null", DIARY_FOLDER);
    #endif
    
    printf("\n=== Current Diary Entries ===\n");
    
    fp = popen(command, "r");
    if (fp == NULL) {
        printf("Could not execute command!\n");
        return;
    }
    
    char filename[MAX_TITLE_LENGTH + 100];
    int count = 0;
    
    while (fgets(filename, sizeof(filename), fp) != NULL) {
        filename[strcspn(filename, "\n")] = 0; // Remove newline character
        
        // Get only filename from full path
        char *baseFileName = strrchr(filename, '\\');
        if (baseFileName != NULL) {
            baseFileName++; // Get part after \ character
        } else {
            baseFileName = filename; // If \ not found, use entire string
        }
        
        // Remove .txt extension and show only title
        char *extension = strstr(baseFileName, ".txt");
        if (extension != NULL) {
            *extension = '\0';
        }
        
        printf("%s\n", baseFileName);
        count++;
    }
    
    pclose(fp);
    
    if (count == 0) {
        printf("No diary entries found!\n");
    }
}

// Read a specific diary entry
void readEntry(const char* filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("File not found!\n");
        return;
    }
    
    DiaryEntry entry;
    fread(&entry, sizeof(DiaryEntry), 1, file);
    fclose(file);
    
    char password[MAX_PASSWORD_LENGTH];
    printf("Password: ");
    fgets(password, MAX_PASSWORD_LENGTH, stdin);
    password[strcspn(password, "\n")] = 0; // Remove newline character
    
    // Encrypt password and compare
    encrypt(password, ENCRYPTION_KEY);
    
    if (strcmp(password, entry.password) != 0) {
        printf("Incorrect password!\n");
        return;
    }
    
    // Decrypt content
    char decryptedContent[MAX_CONTENT_LENGTH];
    strcpy(decryptedContent, entry.content);
    decrypt(decryptedContent, ENCRYPTION_KEY);
    
    printf("\n=== %s (%s) ===\n", entry.title, entry.date);
    printf("%s", decryptedContent);
}

// Delete diary entry
void deleteEntry(const char* filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("File not found!\n");
        return;
    }
    
    DiaryEntry entry;
    fread(&entry, sizeof(DiaryEntry), 1, file);
    fclose(file);
    
    char password[MAX_PASSWORD_LENGTH];
    printf("Password: ");
    fgets(password, MAX_PASSWORD_LENGTH, stdin);
    password[strcspn(password, "\n")] = 0; // Remove newline character
    
    // Encrypt password and compare
    encrypt(password, ENCRYPTION_KEY);
    
    if (strcmp(password, entry.password) != 0) {
        printf("Incorrect password!\n");
        return;
    }
    
    if (remove(filename) == 0) {
        printf("Diary entry successfully deleted!\n");
    } else {
        printf("Could not delete diary entry!\n");
    }
}

// Metni şifreleme (XOR yöntemi)
void encrypt(char* text, int key) {
    for (int i = 0; text[i] != '\0'; i++) {
        text[i] = text[i] ^ key;
    }
}

// Metni deşifre etme (XOR yöntemi)
void decrypt(char* text, int key) {
    encrypt(text, key); // XOR şifrelemede şifreleme ve deşifreleme aynı işlem
}

// Güncel tarihi alma
void getCurrentDate(char* dateStr) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(dateStr, 20, "%d-%m-%Y %H:%M", tm);
} */