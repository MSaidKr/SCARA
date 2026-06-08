import cv2
import numpy as np
import requests
import math
import time

# --- 1. AYARLAR ---
L1 = 10.3  # Omuz - Dirsek uzunluğu (cm)
L2 = 14.3  # Dirsek - Manipülatör uzunluğu (cm)
PIKSEL_BASINA_CM = 0.1 

# IP Adresleri (Kendi bulduğun doğru IP adresi)
YAYIN_URL = 'http://10.23.212.230:81/stream'
KOMUT_URL = 'http://10.23.212.230:8080/adim'

def acilari_hesapla(x_cm, y_cm):
    mesafe = math.sqrt(x_cm**2 + y_cm**2)
    if mesafe > (L1 + L2) or mesafe < abs(L1 - L2):
        return None, None # Hedef menzil dışı
        
    D = (x_cm**2 + y_cm**2 - L1**2 - L2**2) / (2 * L1 * L2)
    theta2 = math.acos(D) 
    
    theta1 = math.atan2(y_cm, x_cm) - math.atan2(L2 * math.sin(theta2), L1 + L2 * math.cos(theta2))
    return math.degrees(theta1), math.degrees(theta2)

# --- 2. KAMERAYI BEKLEME DÖNGÜSÜ (ÇÖKMEYİ ÖNLER) ---
print("Kamera araniyor... SCARA uykuda olabilir.")
print("Lutfen robotu uyandirmak icin elinizi sallayin!")

while True:
    cap = cv2.VideoCapture(YAYIN_URL)
    if cap.isOpened():
        print("Gozler acildi! Goruntu aliniyor...")
        break
    
    print("Kamera kapali. 2 saniye sonra tekrar denenecek...")
    time.sleep(2)

# --- 3. ANA GÖREV (GÖRÜNTÜ İŞLEME VE HAREKET) ---
while True:
    ret, frame = cap.read()
    if not ret: continue

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    _, thresh = cv2.threshold(gray, 100, 255, cv2.THRESH_BINARY_INV)
    contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    for cnt in contours:
        if cv2.contourArea(cnt) > 500:
            M = cv2.moments(cnt)
            if M["m00"] != 0:
                cx = int(M["m10"] / M["m00"])
                cy = int(M["m01"] / M["m00"])
                
                x_cm = (cx - 160) * PIKSEL_BASINA_CM
                y_cm = (240 - cy) * PIKSEL_BASINA_CM 
                
                omuz_derece, dirsek_derece = acilari_hesapla(x_cm, y_cm)
                
                if omuz_derece is not None:
                    omuz_adim = int((omuz_derece / 360.0) * 2048)
                    dirsek_adim = int((dirsek_derece / 360.0) * 2048)
                    
                    istek_linki = f"{KOMUT_URL}?o={omuz_adim}&d={dirsek_adim}"
                    try:
                        requests.get(istek_linki, timeout=0.1) 
                    except:
                        pass 
                        
                    cv2.putText(frame, f"Omuz:{omuz_adim} Dirsek:{dirsek_adim}", (cx - 40, cy - 20),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
                    
                cv2.circle(frame, (cx, cy), 5, (0, 0, 255), -1)

    cv2.imshow("SCARA Yapay Zeka", frame)
    if cv2.waitKey(1) & 0xFF == ord('q'): break

cap.release()
cv2.destroyAllWindows()