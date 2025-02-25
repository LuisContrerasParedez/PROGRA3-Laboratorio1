#include <iostream>
#include <string>
#include <cstring>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <chrono>
#include <set>
#include <thread>
#include <limits>

#include "bass.h"

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#endif

using namespace std;
namespace fs = std::filesystem;

// --------------------------------------------------------------------------------------
// Estructura de la lista
// --------------------------------------------------------------------------------------
struct Nodo
{
    int id;                 
    char nombreCancion[200]; 
    char artista[100];       
    float duracion;          
    char genero[50];         
    Nodo *sgte;
};

typedef Nodo *Tlista;

void parseID3v2Tags(const char *id3v2, string &artista, string &genero)
{

    const char *ptr = id3v2;
    while (*ptr)
    {
        string campo = ptr;
        ptr += (campo.size() + 1); 

        if (campo.rfind("TPE1=", 0) == 0) 
        {
            artista = campo.substr(5); 
        }
        else if (campo.rfind("TCON=", 0) == 0) 
        {
            genero = campo.substr(5); 
        }
    }
}

// Extrae metadatos desde un HSTREAM (BASS_STREAM_DECODE)
void leerMetadatosID3(HSTREAM stream, string &artista, string &genero)
{
    const TAG_ID3 *id3v1 = (const TAG_ID3 *)BASS_ChannelGetTags(stream, BASS_TAG_ID3);
    if (id3v1)
    {
        artista = id3v1->artist;
        unsigned char genreIndex = (unsigned char)id3v1->genre;
        genero = "ID3v1_genre" + to_string(genreIndex);
        return; 
    }

    const char *id3v2 = (const char *)BASS_ChannelGetTags(stream, BASS_TAG_ID3V2);
    if (id3v2)
    {
        parseID3v2Tags(id3v2, artista, genero);
    }
}

// --------------------------------------------------------------------------------------
void obtenerInfoCancion(const string &ruta, float &duracionMin, string &artista, string &genero)
{
    // Inicializa BASS en modo decode (para leer info)
    if (!BASS_Init(-1, 44100, 0, 0, NULL))
    {
        cerr << "Error al inicializar BASS para obtener info de: " << ruta << endl;
        duracionMin = 0.0f;
        artista = "Desconocido";
        genero = "Desconocido";
        return;
    }

    HSTREAM stream = BASS_StreamCreateFile(FALSE, ruta.c_str(), 0, 0, BASS_STREAM_DECODE);
    if (!stream)
    {
        cerr << "No se pudo abrir la cancion (para info): " << ruta << endl;
        duracionMin = 0.0f;
        artista = "Desconocido";
        genero = "Desconocido";
        BASS_Free();
        return;
    }

    // 1) Duracion
    QWORD lengthInBytes = BASS_ChannelGetLength(stream, BASS_POS_BYTE);
    double durationSeconds = BASS_ChannelBytes2Seconds(stream, lengthInBytes);
    duracionMin = static_cast<float>(durationSeconds / 60.0);

    // 2) Metadatos (ID3)
    string tempArtista, tempGenero;
    leerMetadatosID3(stream, tempArtista, tempGenero);

    artista = tempArtista.empty() ? "Desconocido" : tempArtista;
    genero = tempGenero.empty() ? "Desconocido" : tempGenero;

    // Liberar
    BASS_StreamFree(stream);
    BASS_Free();
}

// --------------------------------------------------------------------------------------
// Descarga la cancion usando yt-dlp
// --------------------------------------------------------------------------------------
bool descargarCancion(const string &nombre, const string &artista)
{
    // Generar nombre de archivo deseado
    string nombreArchivo = nombre + " - " + artista + ".mp3";

    // Reemplazar caracteres no validos en nombres de archivo
    for (char &c : nombreArchivo)
    {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
        {
            c = '_'; 
        }
    }

    // Comando para descargar (yt-dlp asignara su propio nombre automaticamente)
    string comando = "yt-dlp.exe -x --audio-format mp3 -o \"musica/%(title)s.%(ext)s\" \"ytsearch1:" + nombre + " " + artista + "\"";

    cout << "Ejecutando comando de descarga: " << comando << endl;
    int resultado = system(comando.c_str());

    if (resultado != 0)
    {
        cerr << "Error al descargar la cancion." << endl;
        return false;
    }

    // Buscar el archivo recien descargado (el mas reciente en la carpeta)
    string carpetaMusica = "musica/";
    string archivoDescargado;
    fs::file_time_type ultimaFecha;

    for (const auto &entry : fs::directory_iterator(carpetaMusica))
    {
        if (entry.path().extension() == ".mp3")
        {
            auto ftime = fs::last_write_time(entry);
            if (archivoDescargado.empty() || ftime > ultimaFecha)
            {
                ultimaFecha = ftime;
                archivoDescargado = entry.path().filename().string();
            }
        }
    }

    if (archivoDescargado.empty())
    {
        cerr << "Error: No se pudo encontrar el archivo descargado." << endl;
        return false;
    }

    // Renombrar el archivo al formato "Nombre - Artista.mp3"
    string rutaOriginal = carpetaMusica + archivoDescargado;
    string rutaNueva = carpetaMusica + nombreArchivo;

    try
    {
        fs::rename(rutaOriginal, rutaNueva);
        cout << "Cancion renombrada a: " << rutaNueva << endl;
        return true;
    }
    catch (const fs::filesystem_error &e)
    {
        cerr << "Error al renombrar el archivo: " << e.what() << endl;
        return false;
    }
}

// --------------------------------------------------------------------------------------
// Insercion en la lista enlazada
// --------------------------------------------------------------------------------------
void insertarEnPosicion(
    Tlista &lista,
    int &idCounter,
    const string &nombreArchivo,
    const string &artista,
    float duracion,
    const string &genero,
    int pos)
{
    Tlista nuevo = new Nodo;
    nuevo->id = idCounter++;

    // Ruta en "musica/"
    string rutaCompleta = "musica/" + nombreArchivo;
    strcpy(nuevo->nombreCancion, rutaCompleta.c_str());
    strcpy(nuevo->artista, artista.c_str());
    nuevo->duracion = duracion;
    strcpy(nuevo->genero, genero.c_str());

    nuevo->sgte = nullptr;

    // Caso: lista vacia o insertar al inicio
    if (pos <= 1 || lista == nullptr)
    {
        nuevo->sgte = lista;
        lista = nuevo;
        return;
    }

    // Insertar en medio/final
    Tlista temp = lista;
    int i = 1;
    while (temp->sgte != nullptr && i < pos - 1)
    {
        temp = temp->sgte;
        i++;
    }

    nuevo->sgte = temp->sgte;
    temp->sgte = nuevo;
}

// --------------------------------------------------------------------------------------
// Mostrar la lista de canciones
// --------------------------------------------------------------------------------------
void mostrarLista(Tlista lista)
{
    if (lista == nullptr)
    {
        cout << "No hay canciones en la lista." << endl;
        return;
    }

    cout << "\nLista de canciones disponibles:\n";
    while (lista != nullptr)
    {
        cout << lista->id << " - "
             << lista->nombreCancion << " | "
             << lista->artista << " | "
             << lista->genero << " | "
             << lista->duracion << " min\n";

        lista = lista->sgte;
    }
}

// --------------------------------------------------------------------------------------
// Buscar cancion por ID
// --------------------------------------------------------------------------------------
Tlista buscarCancion(Tlista lista, int id)
{
    while (lista != nullptr)
    {
        if (lista->id == id)
        {
            return lista;
        }
        lista = lista->sgte;
    }
    return nullptr;
}

// --------------------------------------------------------------------------------------
// Eliminar cancion de la lista
// --------------------------------------------------------------------------------------
void eliminarCancion(Tlista &lista, int id)
{
    Tlista temp = lista, anterior = nullptr;

    while (temp != nullptr && temp->id != id)
    {
        anterior = temp;
        temp = temp->sgte;
    }

    if (temp == nullptr)
    {
        cout << "La cancion con ID " << id << " no se encontro." << endl;
        return;
    }

    if (anterior == nullptr)
    {
        lista = temp->sgte;
    }
    else
    {
        anterior->sgte = temp->sgte;
    }

    delete temp;
    cout << "Cancion eliminada con exito." << endl;
}

// --------------------------------------------------------------------------------------
// Reproducir cancion con actualizacion en tiempo real
// --------------------------------------------------------------------------------------
void reproducirCancion(const char *ruta, const char *artista, float duracion, const char *genero)
{
    // Inicializa BASS
    if (!BASS_Init(-1, 44100, 0, 0, NULL))
    {
        cerr << "Error al inicializar BASS para reproducir." << endl;
        return;
    }

    HSTREAM stream = BASS_StreamCreateFile(FALSE, ruta, 0, 0, 0);
    if (!stream)
    {
        cerr << "Error al cargar el archivo: " << ruta << endl;
        BASS_Free();
        return;
    }

    // Duracion real en segundos
    QWORD lengthInBytes = BASS_ChannelGetLength(stream, BASS_POS_BYTE);
    double totalSegundos = BASS_ChannelBytes2Seconds(stream, lengthInBytes);

    cout << "----------------------------------------\n";
    cout << "Reproduciendo: " << ruta << endl;
    cout << "Artista: " << artista << endl;
    cout << "Genero: " << genero << endl;
    cout << "Duracion (aprox.): " << duracion << " minutos\n";
    cout << "(Segun BASS, " << totalSegundos << " segundos)\n";
    cout << "----------------------------------------\n";

    BASS_ChannelPlay(stream, FALSE);

    bool detener = false;
    bool enPausa = false;
    bool repetir = false;

    cout << "\n[Controles: P = Pausar/Reanudar, R = Repetir ON/OFF, S = Parar/Salir]\n"
         << endl;

    // Bucle de reproduccion
    while (!detener)
    {
        if (!enPausa)
        {
            // Obtenemos la posicion actual
            QWORD pos = BASS_ChannelGetPosition(stream, BASS_POS_BYTE);
            double segTranscurridos = BASS_ChannelBytes2Seconds(stream, pos);

            // Convertir a mm:ss
            int minAct = static_cast<int>(segTranscurridos / 60);
            int segAct = static_cast<int>(segTranscurridos) % 60;
            int minTot = static_cast<int>(totalSegundos / 60);
            int segTot = static_cast<int>(totalSegundos) % 60;

            cout << "\rTiempo: "
                 << minAct << ":" << (segAct < 10 ? "0" : "") << segAct
                 << " / "
                 << minTot << ":" << (segTot < 10 ? "0" : "") << segTot
                 << "   ";
            cout.flush();

            // Si la cancion termino (BASS_ACTIVE_STOPPED) y no esta en pausa
            if (BASS_ChannelIsActive(stream) == BASS_ACTIVE_STOPPED)
            {
                if (repetir)
                {
                    // Reiniciar
                    BASS_ChannelSetPosition(stream, 0, BASS_POS_BYTE);
                    BASS_ChannelPlay(stream, FALSE);
                }
                else
                {
                    detener = true;
                }
            }
        }

#ifdef _WIN32
        // Revisa si se presiono una tecla
        if (_kbhit())
        {
            char c = toupper(_getch());
            switch (c)
            {
            case 'P':
                if (!enPausa)
                {
                    BASS_ChannelPause(stream);
                    enPausa = true;
                }
                else
                {
                    BASS_ChannelPlay(stream, FALSE);
                    enPausa = false;
                }
                break;

            case 'R':
                repetir = !repetir;
                if (repetir)
                    cout << "\n  [Repeticion ACTIVADA]\n";
                else
                    cout << "\n  [Repeticion DESACTIVADA]\n";
                break;

            case 'S':
                detener = true;
                break;
            }
        }
    
        Sleep(1000);
#endif
    }

    BASS_ChannelStop(stream);
    BASS_StreamFree(stream);
    BASS_Free();

    cout << "\nReproduccion finalizada.\n"
         << endl;
}

// --------------------------------------------------------------------------------------
// Cargar canciones existentes en "musica/" y asignar metadatos
// --------------------------------------------------------------------------------------
void cargarCancionesDesdeCarpeta(Tlista &lista, int &idCounter)
{
    string carpeta = "musica/";
    for (const auto &entry : fs::directory_iterator(carpeta))
    {
        if (entry.path().extension() == ".mp3")
        {
            string ruta = entry.path().string();                     // ruta completa
            string nombreArchivo = entry.path().filename().string(); // "archivo.mp3"

            // Obtener info (duracion, artista, genero)
            float durMin;
            string artista, genero;
            obtenerInfoCancion(ruta, durMin, artista, genero);

            // Insertar en la lista con la posicion = idCounter (al final)
            insertarEnPosicion(
                lista,
                idCounter,
                nombreArchivo,
                artista,
                (durMin > 0.0f ? durMin : 3.5f),
                genero,
                idCounter);
        }
    }
    cout << "Canciones cargadas desde la carpeta 'musica/'." << endl;
}

// --------------------------------------------------------------------------------------
// Menu principal
// --------------------------------------------------------------------------------------
void menu()
{
    cout << "\n--- REPRODUCTOR DE MUSICA ---\n";
    cout << "1. Listar canciones\n";
    cout << "2. Reproducir una cancion\n";
    cout << "3. Eliminar una cancion\n";
    cout << "4. Agregar una nueva cancion (descargar con yt-dlp)\n";
    cout << "5. Salir\n";
    cout << "Seleccione una opcion: ";
}

// --------------------------------------------------------------------------------------
// main
// --------------------------------------------------------------------------------------
int main()
{
    Tlista lista = nullptr;
    int opcion, id, pos;
    int idCounter = 1;

    // Cargar canciones existentes en "musica/"
    cargarCancionesDesdeCarpeta(lista, idCounter);

    do
    {
        menu();
        cin >> opcion;
        cin.ignore(std::numeric_limits<streamsize>::max(), '\n');

        switch (opcion)
        {
        case 1:
        {
            mostrarLista(lista);
            break;
        }
        case 2:
        {
            cout << "Ingrese el ID de la cancion a reproducir: ";
            cin >> id;
            cin.ignore(std::numeric_limits<streamsize>::max(), '\n');
            cout << "\n";

            Tlista cancion = buscarCancion(lista, id);
            if (cancion)
            {
                reproducirCancion(
                    cancion->nombreCancion,
                    cancion->artista,
                    cancion->duracion,
                    cancion->genero);
            }
            else
            {
                cout << "Cancion no encontrada." << endl;
            }
            break;
        }
        case 3:
        {
            cout << "Ingrese el ID de la cancion a eliminar: ";
            cin >> id;
            cin.ignore(std::numeric_limits<streamsize>::max(), '\n');
            eliminarCancion(lista, id);
            break;
        }
        case 4:
        {
            string nombreCancion, artista;

            // Pedir datos al usuario
            cout << "Ingrese el titulo de la cancion a buscar/descargar: ";
            getline(cin, nombreCancion);
            cout << "Ingrese el artista: ";
            getline(cin, artista);

            // Construir el nombre esperado
            string nombreArchivo = nombreCancion + " - " + artista + ".mp3";

            // Reemplazar caracteres invalidos
            for (char &c : nombreArchivo)
            {
                if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
                {
                    c = '_';
                }
            }

            cout << "Ingrese la posicion en la que quiere agregar la cancion: ";
            cin >> pos;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            // Descargar y renombrar
            if (descargarCancion(nombreCancion, artista))
            {
                string ruta = "musica/" + nombreArchivo;
                float durMin;
                string artistaTag, generoTag;
                obtenerInfoCancion(ruta, durMin, artistaTag, generoTag);

                // Insertar en la lista con el nombre correcto
                insertarEnPosicion(
                    lista,
                    idCounter,
                    nombreArchivo,
                    artistaTag,
                    (durMin > 0.0f ? durMin : 3.5f),
                    generoTag,
                    pos);

                cout << "Cancion agregada con exito en la posicion " << pos << " con el nombre: " << nombreArchivo << endl;
            }
            else
            {
                cerr << "Error al descargar la cancion." << endl;
            }

            break;
        }
        case 5:
        {
            cout << "Saliendo...\n";
            break;
        }
        default:
            cout << "Opcion no valida.\n";
        }

    } while (opcion != 5);

    return 0;
}
