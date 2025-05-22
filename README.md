# CryptoDiary

CryptoDiary, terminal üzerinde çalışan basit bir günlük uygulamasıdır. Kullanıcıların özel günlük notları yazmasına ve bunları şifrelenmiş bir şekilde kaydetmesine olanak tanır. Her not bir şifre ile korunur ve yalnızca şifreyi bilen kişi tarafından okunabilir.

## Özellikler

- Günlük notları yazma ve basit bir şifreleme yöntemi (XOR) kullanarak kaydetme
- Notları görüntülemeden önce doğru şifreyi sorma
- Kaydedilmiş tüm notları listeleme ve silme imkanı

## Nasıl Kullanılır

1. Programı derlemek için:
   ```
   gcc cryptodiary.c -o cryptodiary
   ```

2. Programı çalıştırmak için:
   ```
   ./cryptodiary
   ```

3. Ana menüden istediğiniz işlemi seçin:
   - Yeni günlük girişi oluştur
   - Günlük girişlerini görüntüle
   - Günlük girişi sil
   - Çıkış

## Teknik Detaylar

- Program, dosya işlemleri, şifreleme ve bellek yönetimi gibi temel C programlama kavramlarını kullanır.
- Günlük girişleri, başlık adına göre .dat uzantılı dosyalarda saklanır.
- Şifreleme için basit XOR yöntemi kullanılmıştır.
- Her günlük girişi, başlık, içerik, şifre ve tarih bilgilerini içerir.

## Gereksinimler

- C derleyicisi (GCC önerilir)
- Standart C kütüphaneleri 