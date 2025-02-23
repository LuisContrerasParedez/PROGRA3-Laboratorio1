🎵 Reproductor de Música en C++ 🎶

📌 Introducción
Este proyecto es un reproductor de música en C++ que permite a los usuarios:
✅ Reproducir música con controles avanzados.

✅ Descargar canciones automáticamente desde YouTube con yt-dlp.

✅ Organizar canciones en una lista enlazada con inserciones en posiciones específicas.

✅ Eliminar canciones de la lista.

✅ Gestionar una biblioteca musical almacenada localmente.

🔹 Está diseñado para funcionar en entornos de Windows 

🚀 Tecnologías Utilizadas
📌 Lenguaje: C++

📌 Audio: BASS Audio Library (bass.dll)

📌 Descargas: yt-dlp

📌 Manejo de archivos: std::filesystem

📌 Estructura de datos: Lista enlazada

📌 Multitarea: std::thread para gestión de tiempos de espera


📂 Estructura del Proyecto
📦 Proyecto-Reproductor  
 ┣ 📂 musica/              # Carpeta donde se almacenan las canciones descargadas  
 
 ┣ 📜 main.cpp             # Código principal del proyecto  
 
 ┣ 📜 bass.dll             # Biblioteca BASS para reproducción de audio 
 
 ┣ 📜 main.exe             # Ejecutable del proyecto
 
 ┣ 📜 yt-dlp.exe           # Ejecutable para descarga de música  
 
 ┣ 📜 ffmpeg.exe           # Ejecutable que es util si yt-dlp lo usa para convertir el audio.
 
 ┣ 📜 README.md            # Documentación del proyecto  
 
 ┗ 📜 comandos.txt         # Archivo con ejemplos de comandos de ejecución  
 
🛠 Instalación y Configuración
✅ 1. Descargar los archivos necesarios
📌 Clonar el repositorio o descargar los archivos manualmente.

✅ 2. Instalar yt-dlp y BASS
📌 yt-dlp: Descarga el ejecutable y colócalo en la carpeta del proyecto.


✅Instalación de BASS (Librería de Audio)
📌 https://www.un4seen.com/files/bass24.zip


✅Instalación de yt-dlp
📌 https://github.com/yt-dlp/yt-dlp/releases/latest


✅Descarga FFmpeg (necesario para convertir a MP3)
📌 https://www.gyan.dev/ffmpeg/builds/ffmpeg-git-full.7z


✅ 3. Compilar el Proyecto
g++ main.cpp -o main.exe -I C:\MinGW\include -L . -lbass

✅ 4. Ejecutar el Programa
./main.exe
