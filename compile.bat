@echo off
echo CryptoDiary derleniyor...
gcc cryptodiary.c -o cryptodiary.exe
if %ERRORLEVEL% EQU 0 (
    echo Derleme başarılı! cryptodiary.exe dosyası oluşturuldu.
    echo Programı çalıştırmak için cryptodiary.exe dosyasına çift tıklayın veya komut satırından cryptodiary yazın.
) else (
    echo Derleme sırasında bir hata oluştu!
)
pause 