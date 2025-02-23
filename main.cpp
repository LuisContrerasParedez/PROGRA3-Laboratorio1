#include <iostream>
#include <string>
#include <cstring>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include "bass.h"
#include <chrono>
#include <thread>

using namespace std;
namespace fs = std::filesystem;

// Definir la estructura del nodo
struct Nodo {
    int id;
    char nombreCancion[200];
    char artista[100];
    float duracion; // en minutos
    Nodo* sgte;
};

typedef Nodo* Tlista;

// Funcion para obtener el ultimo archivo descargado en la carpeta "musica"
string obtenerUltimoArchivoDescargado() {
    string ultimoArchivo = "";
    time_t ultimoTiempo = 0;
    string carpetaMusica = "musica/";

    for (const auto& entry : fs::directory_iterator(carpetaMusica)) {
        if (entry.path().extension() == ".mp3") {
            auto tiempoModificacion = fs::last_write_time(entry.path());
            auto tiempoArchivo = chrono::duration_cast<chrono::seconds>(tiempoModificacion.time_since_epoch()).count();

            if (tiempoArchivo > ultimoTiempo) {
                ultimoTiempo = tiempoArchivo;
                ultimoArchivo = entry.path().filename().string();
            }
        }
    }
    return ultimoArchivo;
}


// Descargar cancion automaticamente con `yt-dlp`
bool descargarCancion(const string& nombre, const string& artista) {
    string busqueda = "ytsearch:" + nombre + " " + artista;
    string comando = "yt-dlp.exe -x --audio-format mp3 -o \"musica/%(title)s.%(ext)s\" \"" + busqueda + "\"";
    int resultado = system(comando.c_str());
    return resultado == 0;
}

// Insertar cancion en la lista en una posicion especifica
void insertarEnPosicion(Tlista &lista, int &idCounter, const string& nombreArchivo, const string& artista, float duracion, int pos) {
    Tlista nuevo = new Nodo;
    nuevo->id = idCounter++;

    // Guardar la ruta del archivo descargado
    string rutaArchivo = "musica/" + nombreArchivo;
    strcpy(nuevo->nombreCancion, rutaArchivo.c_str());
    strcpy(nuevo->artista, artista.c_str());
    nuevo->duracion = duracion;
    nuevo->sgte = nullptr;

    if (pos == 1 || lista == nullptr) { 
        nuevo->sgte = lista;
        lista = nuevo;
    } else {
        Tlista temp = lista;
        int i = 1;
        while (temp->sgte != nullptr && i < pos - 1) {
            temp = temp->sgte;
            i++;
        }
        nuevo->sgte = temp->sgte;
        temp->sgte = nuevo;
    }
}

// Mostrar lista de canciones
void mostrarLista(Tlista lista) {
    if (lista == nullptr) {
        cout << "No hay canciones en la lista." << endl;
        return;
    }

    cout << "\nLista de canciones disponibles:\n";
    while (lista != nullptr) {
        cout << lista->id << " - " << lista->nombreCancion << " | " << lista->artista << " | " << lista->duracion << " min\n";
        lista = lista->sgte;
    }
}

// Buscar cancion por ID
Tlista buscarCancion(Tlista lista, int id) {
    while (lista != nullptr) {
        if (lista->id == id) {
            return lista;
        }
        lista = lista->sgte;
    }
    return nullptr;
}

// Eliminar cancion de la lista
void eliminarCancion(Tlista &lista, int id) {
    Tlista temp = lista, anterior = nullptr;

    while (temp != nullptr && temp->id != id) {
        anterior = temp;
        temp = temp->sgte;
    }

    if (temp == nullptr) {
        cout << "La cancion con ID " << id << " no se encontro." << endl;
        return;
    }

    if (anterior == nullptr) {
        lista = temp->sgte;
    } else {
        anterior->sgte = temp->sgte;
    }

    delete temp;
    cout << "Cancion eliminada con exito." << endl;
}

// Reproducir cancion
void reproducirCancion(const char* ruta, const char* artista, float duracion) {
    if (!BASS_Init(-1, 44100, 0, 0, NULL)) {
        cerr << "Error al inicializar BASS" << endl;
        return;
    }

    HSTREAM stream = BASS_StreamCreateFile(FALSE, ruta, 0, 0, 0);
    if (!stream) {
        cerr << "Error al cargar el archivo: " << ruta << endl;
        BASS_Free();
        return;
    }

    cout << "Reproduciendo: " << ruta << endl;
    cout << "Artista: " << artista << endl;
    cout << "Duracion: " << duracion << " minutos" << endl;

    BASS_ChannelPlay(stream, FALSE);
    cout << "Presiona ENTER para detener la musica..." << endl;
    cin.get();

    BASS_ChannelStop(stream);
    BASS_StreamFree(stream);
    BASS_Free();
}

// Cargar canciones desde la carpeta "musica/"
void cargarCancionesDesdeCarpeta(Tlista &lista, int &idCounter) {
    string carpetaMusica = "musica/";

    for (const auto& entry : fs::directory_iterator(carpetaMusica)) {
        if (entry.path().extension() == ".mp3") {
            insertarEnPosicion(lista, idCounter, entry.path().filename().string(), "Desconocido", 3.5, idCounter);
        }
    }

    cout << "Canciones cargadas desde la carpeta 'musica/'." << endl;
}

// Menu principal
void menu() {
    cout << "\n--- REPRODUCTOR DE MUSICA ---\n";
    cout << "1. Listar canciones\n";
    cout << "2. Reproducir una cancion\n";
    cout << "3. Eliminar una cancion\n";
    cout << "4. Agregar una nueva cancion\n";
    cout << "5. Salir\n";
    cout << "Seleccione una opcion: ";
}

int main() {
    Tlista lista = nullptr;
    int opcion, id, pos, idCounter = 1;
    string nombreCancion, artista;

    // Cargar canciones desde la carpeta automaticamente al iniciar
    cargarCancionesDesdeCarpeta(lista, idCounter);

    do {
        menu();
        cin >> opcion;
        cin.ignore(); // Limpiar buffer

        switch (opcion) {
            case 1:
                mostrarLista(lista);
                break;

            case 2:
                cout << "Ingrese el ID de la cancion a reproducir: ";
                cin >> id;
                cin.ignore();
                {
                    Tlista cancion = buscarCancion(lista, id);
                    if (cancion) {
                        reproducirCancion(cancion->nombreCancion, cancion->artista, cancion->duracion);
                    } else {
                        cout << "Cancion no encontrada." << endl;
                    }
                }
                break;

            case 3:
                cout << "Ingrese el ID de la cancion a eliminar: ";
                cin >> id;
                eliminarCancion(lista, id);
                break;

                case 4:
                cout << "Ingrese el nombre de la nueva cancion: ";
                getline(cin, nombreCancion);
                cout << "Ingrese el artista: ";
                getline(cin, artista);
                cout << "Ingrese la posicion en la que quiere agregar la cancion: ";
                cin >> pos;
                cin.ignore();
            
                if (descargarCancion(nombreCancion, artista)) {
                    // Esperar un momento para asegurar que el archivo se haya guardado antes de buscarlo
                    this_thread::sleep_for(chrono::seconds(2));
            
                    // Obtener el nombre real del archivo descargado
                    string archivoDescargado = obtenerUltimoArchivoDescargado();
            
                    if (!archivoDescargado.empty()) {
                        insertarEnPosicion(lista, idCounter, "musica/" + archivoDescargado, artista, 3.5, pos);
                        cout << "Cancion descargada y agregada en la posicion " << pos << " de la lista con el nombre real: " << archivoDescargado << endl;
                    } else {
                        cout << "Error: No se encontro el archivo descargado." << endl;
                    }
                } else {
                    cout << "Error al descargar la cancion." << endl;
                }
                break;
        
            case 5:
                cout << "Saliendo del reproductor...\n";
                break;

            default:
                cout << "Opcion no valida. Intente de nuevo.\n";
        }

    } while (opcion != 5);

    return 0;
}
