//Proyecto final :)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TEXTO   512
#define MAX_OPCION  256
#define MAX_NOMBRE  128

typedef struct Reactivo {
    char pregunta[MAX_TEXTO];
    char opcion[4][MAX_OPCION];   
    int  correcta;                
    float puntos;
    int  respuesta_usuario;       /* 0 = sin responder        */

    struct Reactivo *anterior;
    struct Reactivo *siguiente;
} Reactivo;


typedef struct {
    Reactivo *cabeza;
    Reactivo *cola;
    Reactivo *actual;
    int       total;
} ListaReactivos;

/* Lista */
ListaReactivos *crear_lista(void);
void            destruir_lista(ListaReactivos *lista);
Reactivo       *crear_reactivo(void);
void            insertar_al_final(ListaReactivos *lista, Reactivo *r);
void            eliminar_actual(ListaReactivos *lista);

/* Archivos */
void            listar_examenes(void);
int             cargar_examen(ListaReactivos *lista, const char *nombre);
void            guardar_examen(ListaReactivos *lista, const char *nombre);

/* Menú y flujos */
void            menu_principal(void);
void            flujo_generar(void);
void            flujo_modificar(void);
void            flujo_aplicar(void);

/* Edición de un reactivo */
void            editar_reactivo(Reactivo *r);
void            mostrar_reactivo_edicion(ListaReactivos *lista, int num);
void            mostrar_reactivo_quiz(ListaReactivos *lista, int num, int total);

/* Utilidades */
void            limpiar_pantalla(void);
void            pausar(void);
void            limpiar_buffer(void);
char            pedir_navegacion(void);

/* Lista doblemente enlazada*/

ListaReactivos *crear_lista(void)
{
    ListaReactivos *l = (ListaReactivos *)malloc(sizeof(ListaReactivos));
    if (!l) { fprintf(stderr, "Error: sin memoria.\n"); exit(1); }
    l->cabeza = l->cola = l->actual = NULL;
    l->total = 0;
    return l;
}

void destruir_lista(ListaReactivos *lista)
{
    Reactivo *cur = lista->cabeza;
    while (cur) {
        Reactivo *sig = cur->siguiente;
        free(cur);
        cur = sig;
    }
    free(lista);
}

Reactivo *crear_reactivo(void)
{
    Reactivo *r = (Reactivo *)calloc(1, sizeof(Reactivo));
    if (!r) { fprintf(stderr, "Error: sin memoria.\n"); exit(1); }
    return r;
}

void insertar_al_final(ListaReactivos *lista, Reactivo *r)
{
    r->anterior  = lista->cola;
    r->siguiente = NULL;

    if (lista->cola)
        lista->cola->siguiente = r;
    else
        lista->cabeza = r;

    lista->cola = r;
    lista->total++;

    if (!lista->actual)
        lista->actual = r;
}

/* Elimina el nodo 'actual' y avanza al siguiente (o al anterior) */
void eliminar_actual(ListaReactivos *lista)
{
    if (!lista->actual) return;

    Reactivo *r    = lista->actual;
    Reactivo *prev = r->anterior;
    Reactivo *next = r->siguiente;

    if (prev) prev->siguiente = next;
    else      lista->cabeza   = next;

    if (next) next->anterior = prev;
    else      lista->cola    = prev;

    lista->actual = next ? next : prev;
    lista->total--;
    free(r);
}

/* ============================================================
   IMPLEMENTACIÓN – Archivos
   ============================================================ */

/* Lista los archivos .txt del directorio actual que sean exámenes */
void listar_examenes(void)
{
    printf("\n  Archivos de examen disponibles (.txt):\n");
    printf("  (Introduce el nombre sin extension)\n\n");

    /* Usamos popen para listar archivos .txt */
#ifdef _WIN32
    FILE *p = popen("dir /b *.txt 2>nul", "r");
#else
    FILE *p = popen("ls *.txt 2>/dev/null", "r");
#endif
    if (!p) {
        printf("  (No se pudieron listar los archivos)\n");
        return;
    }
    char buf[MAX_NOMBRE];
    int  n = 0;
    while (fgets(buf, sizeof(buf), p)) {
        buf[strcspn(buf, "\r\n")] = '\0';
        printf("  [%d] %s\n", ++n, buf);
    }
    pclose(p);
    if (n == 0) printf("  (Ninguno encontrado)\n");
    printf("\n");
}

/*
 * Carga un archivo con el formato:
 *   :p;Pregunta
 *   :op1;Opcion1
 *   :op2;Opcion2
 *   :op3;Opcion3
 *   :op4;Opcion4
 *   :r;opN
 *   NN.
 *
 * Retorna 1 si tuvo éxito, 0 si falló.
 */
int cargar_examen(ListaReactivos *lista, const char *nombre)
{
    char ruta[MAX_NOMBRE + 8];
    snprintf(ruta, sizeof(ruta), "%s.txt", nombre);

    FILE *f = fopen(ruta, "r");
    if (!f) {
        printf("  Error: no se pudo abrir '%s'.\n", ruta);
        return 0;
    }

    char linea[MAX_TEXTO];
    Reactivo *r = NULL;

    while (fgets(linea, sizeof(linea), f)) {
        linea[strcspn(linea, "\r\n")] = '\0';

        if (strncmp(linea, ":p;", 3) == 0) {
            /* Nuevo reactivo */
            r = crear_reactivo();
            strncpy(r->pregunta, linea + 3, MAX_TEXTO - 1);

        } else if (strncmp(linea, ":op1;", 5) == 0 && r) {
            strncpy(r->opcion[0], linea + 5, MAX_OPCION - 1);

        } else if (strncmp(linea, ":op2;", 5) == 0 && r) {
            strncpy(r->opcion[1], linea + 5, MAX_OPCION - 1);

        } else if (strncmp(linea, ":op3;", 5) == 0 && r) {
            strncpy(r->opcion[2], linea + 5, MAX_OPCION - 1);

        } else if (strncmp(linea, ":op4;", 5) == 0 && r) {
            strncpy(r->opcion[3], linea + 5, MAX_OPCION - 1);

        } else if (strncmp(linea, ":r;", 3) == 0 && r) {
            /* :r;opN  →  N es el número de la opción correcta */
            char *ptr = linea + 3;          /* "opN" */
            if (strncmp(ptr, "op", 2) == 0)
                r->correcta = atoi(ptr + 2);

        } else if (r && strlen(linea) > 0 &&
                   (linea[strlen(linea)-1] == '.' ||
                    (linea[0] >= '0' && linea[0] <= '9'))) {
            /* Línea de puntos: "NN." */
            r->puntos = atof(linea);
            insertar_al_final(lista, r);
            r = NULL;
        }
    }
    /* Si quedó un reactivo sin línea de puntos (archivo mal terminado) */
    if (r) {
        r->puntos = 1.0f;
        insertar_al_final(lista, r);
    }

    fclose(f);
    printf("  Examen '%s' cargado: %d reactivo(s).\n", nombre, lista->total);
    return 1;
}

//Guarda la lista en el archivo con el formato especificado
void guardar_examen(ListaReactivos *lista, const char *nombre)
{
    char ruta[MAX_NOMBRE + 8];
    snprintf(ruta, sizeof(ruta), "%s.txt", nombre);

    FILE *f = fopen(ruta, "w");
    if (!f) {
        printf("  Error: no se pudo guardar '%s'.\n", ruta);
        return;
    }

    Reactivo *cur = lista->cabeza;
    while (cur) {
        fprintf(f, ":p;%s\n",   cur->pregunta);
        fprintf(f, ":op1;%s\n", cur->opcion[0]);
        fprintf(f, ":op2;%s\n", cur->opcion[1]);
        fprintf(f, ":op3;%s\n", cur->opcion[2]);
        fprintf(f, ":op4;%s\n", cur->opcion[3]);
        fprintf(f, ":r;op%d\n", cur->correcta);
        fprintf(f, "%.1f.\n",   cur->puntos);
        cur = cur->siguiente;
    }

    fclose(f);
    printf("  Examen guardado en '%s' (%d reactivo(s)).\n", ruta, lista->total);
}

//IMPLEMENTACIÓN – Utilidades de pantalla
void limpiar_pantalla(void)
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pausar(void)
{
    printf("\n  Presiona ENTER para continuar...");
    limpiar_buffer();
    getchar();
}

void limpiar_buffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* Devuelve 'a'=anterior, 'd'=siguiente, 'q'=salir, 'n'=nuevo,
   'e'=eliminar, 's'=guardar, 'r'=responder */
char pedir_navegacion(void)
{
    char buf[8];
    printf("\n  [A]nterior  [D]erecha/Siguiente  [N]uevo  [E]liminar  [G]uardar  [S]alir\n  Opcion: ");
    fgets(buf, sizeof(buf), stdin);
    return (char)tolower((unsigned char)buf[0]);
}

  //EDICIÓN de un reactivo
void editar_reactivo(Reactivo *r)
{
    char buf[MAX_TEXTO];

    printf("\n  --- Edicion del Reactivo ---\n");

    printf("  Pregunta [actual: %.60s...]\n  > ", r->pregunta);
    fgets(buf, sizeof(buf), stdin);
    buf[strcspn(buf, "\r\n")] = '\0';
    if (strlen(buf) > 0) strncpy(r->pregunta, buf, MAX_TEXTO - 1);

    for (int i = 0; i < 4; i++) {
        printf("  Opcion %d [actual: %s]\n  > ", i+1, r->opcion[i]);
        fgets(buf, sizeof(buf), stdin);
        buf[strcspn(buf, "\r\n")] = '\0';
        if (strlen(buf) > 0) strncpy(r->opcion[i], buf, MAX_OPCION - 1);
    }

    printf("  Respuesta correcta (1-4) [actual: %d]\n  > ", r->correcta);
    fgets(buf, sizeof(buf), stdin);
    buf[strcspn(buf, "\r\n")] = '\0';
    int c = atoi(buf);
    if (c >= 1 && c <= 4) r->correcta = c;

    printf("  Puntos [actual: %.1f]\n  > ", r->puntos);
    fgets(buf, sizeof(buf), stdin);
    buf[strcspn(buf, "\r\n")] = '\0';
    float p = atof(buf);
    if (p > 0) r->puntos = p;

    printf("  Reactivo actualizado.\n");
}

/* Muestra el reactivo actual en modo edición */
void mostrar_reactivo_edicion(ListaReactivos *lista, int num)
{
    Reactivo *r = lista->actual;
    if (!r) { printf("  (Sin reactivos)\n"); return; }

    printf("\n  ----- Reactivo %d / %d -----\n", num, lista->total);
    printf("  P: %s\n\n", r->pregunta);
    for (int i = 0; i < 4; i++)
        printf("  op%d: %s\n", i+1, r->opcion[i]);
    printf("\n  Correcta: op%d  |  Puntos: %.1f\n", r->correcta, r->puntos);
}

/* Muestra el reactivo actual en modo quiz */
void mostrar_reactivo_quiz(ListaReactivos *lista, int num, int total)
{
    Reactivo *r = lista->actual;
    if (!r) return;

    printf("\n  ----- Pregunta %d de %d -----\n\n", num, total);
    printf("  %s\n\n", r->pregunta);
    for (int i = 0; i < 4; i++) {
        char marca = (r->respuesta_usuario == i+1) ? '*' : ' ';
        printf("  %c [%d] %s\n", marca, i+1, r->opcion[i]);
    }
    if (r->respuesta_usuario)
        printf("\n  (Tu respuesta: op%d)\n", r->respuesta_usuario);
    else
        printf("\n  (Sin respuesta)\n");

    printf("\n  [A]nterior  [D]erecha  [1-4] Responder  [F]inalizar\n");
}

   //FLUJOS PRINCIPALES
//Para generar el examen
void flujo_generar(void)
{
    limpiar_pantalla();
    printf("\n  ----- GENERAR EXAMEN -----\n");
    printf("  Nombre del nuevo examen (sin .txt): ");

    char nombre[MAX_NOMBRE];
    fgets(nombre, sizeof(nombre), stdin);
    nombre[strcspn(nombre, "\r\n")] = '\0';
    if (strlen(nombre) == 0) return;

    ListaReactivos *lista = crear_lista();
    int  num = 0;
    char op;

    /* Insertar al menos un reactivo para empezar */
    Reactivo *r = crear_reactivo();
    strcpy(r->pregunta,  "Escribe tu pregunta aqui");
    strcpy(r->opcion[0], "Opcion 1");
    strcpy(r->opcion[1], "Opcion 2");
    strcpy(r->opcion[2], "Opcion 3");
    strcpy(r->opcion[3], "Opcion 4");
    r->correcta = 1;
    r->puntos   = 1.0f;
    insertar_al_final(lista, r);
    num = 1;

    editar_reactivo(lista->actual);

    do {
        limpiar_pantalla();
        mostrar_reactivo_edicion(lista, num);

        op = pedir_navegacion();

        if (op == 'a') {
            if (lista->actual->anterior) {
                lista->actual = lista->actual->anterior;
                num--;
            } else printf("  Ya estas en el primer reactivo.\n");

        } else if (op == 'd') {
            if (lista->actual->siguiente) {
                lista->actual = lista->actual->siguiente;
                num++;
            } else printf("  Ya estas en el ultimo reactivo.\n");

        } else if (op == 'n') {
            /* Nuevo reactivo al final */
            Reactivo *nuevo = crear_reactivo();
            strcpy(nuevo->pregunta,  "Nueva pregunta");
            strcpy(nuevo->opcion[0], "Opcion 1");
            strcpy(nuevo->opcion[1], "Opcion 2");
            strcpy(nuevo->opcion[2], "Opcion 3");
            strcpy(nuevo->opcion[3], "Opcion 4");
            nuevo->correcta = 1;
            nuevo->puntos   = 1.0f;
            insertar_al_final(lista, nuevo);
            /* Ir al nuevo */
            lista->actual = lista->cola;
            num = lista->total;
            limpiar_pantalla();
            editar_reactivo(lista->actual);

        } else if (op == 'e') {
            if (lista->total == 1) {
                printf("  No puedes eliminar el unico reactivo.\n");
                pausar();
            } else {
                eliminar_actual(lista);
                num = (num > lista->total) ? lista->total : num;
                printf("  Reactivo eliminado.\n");
                pausar();
            }

        } else if (op == 'g') {
            guardar_examen(lista, nombre);
            pausar();

        } else if (op == 's') {
            printf("  Guardar antes de salir? (s/n): ");
            char yn[4]; fgets(yn, sizeof(yn), stdin);
            if (tolower((unsigned char)yn[0]) == 's')
                guardar_examen(lista, nombre);
        }

    } while (op != 's');

    destruir_lista(lista);
}

//Modificar el examen
void flujo_modificar(void)
{
    limpiar_pantalla();
    printf("\n  ===== MODIFICAR EXAMEN =====\n");
    listar_examenes();
    printf("  Nombre del examen a modificar (sin .txt): ");

    char nombre[MAX_NOMBRE];
    fgets(nombre, sizeof(nombre), stdin);
    nombre[strcspn(nombre, "\r\n")] = '\0';
    if (strlen(nombre) == 0) return;

    ListaReactivos *lista = crear_lista();
    if (!cargar_examen(lista, nombre)) {
        pausar();
        destruir_lista(lista);
        return;
    }
    pausar();

    int  num = 1;
    char op;

    do {
        limpiar_pantalla();
        mostrar_reactivo_edicion(lista, num);

        printf("\n  [A]nterior  [D]siguiente  [N]uevo  [E]ditar  [X]eliminar  [G]uardar  [S]alir\n  Opcion: ");
        char buf[8];
        fgets(buf, sizeof(buf), stdin);
        op = (char)tolower((unsigned char)buf[0]);

        if (op == 'a') {
            if (lista->actual->anterior) { lista->actual = lista->actual->anterior; num--; }
            else { printf("  Primer reactivo.\n"); pausar(); }

        } else if (op == 'd') {
            if (lista->actual->siguiente) { lista->actual = lista->actual->siguiente; num++; }
            else { printf("  Ultimo reactivo.\n"); pausar(); }

        } else if (op == 'n') {
            Reactivo *nuevo = crear_reactivo();
            strcpy(nuevo->pregunta,  "Nueva pregunta");
            strcpy(nuevo->opcion[0], "Opcion 1");
            strcpy(nuevo->opcion[1], "Opcion 2");
            strcpy(nuevo->opcion[2], "Opcion 3");
            strcpy(nuevo->opcion[3], "Opcion 4");
            nuevo->correcta = 1;
            nuevo->puntos   = 1.0f;
            insertar_al_final(lista, nuevo);
            lista->actual = lista->cola;
            num = lista->total;
            limpiar_pantalla();
            editar_reactivo(lista->actual);

        } else if (op == 'e') {
            limpiar_pantalla();
            editar_reactivo(lista->actual);

        } else if (op == 'x') {
            if (lista->total == 1) { printf("  No puedes eliminar el unico reactivo.\n"); pausar(); }
            else {
                eliminar_actual(lista);
                if (num > lista->total) num = lista->total;
                printf("  Reactivo eliminado.\n"); pausar();
            }

        } else if (op == 'g') {
            guardar_examen(lista, nombre);
            pausar();

        } else if (op == 's') {
            printf("  Guardar antes de salir? (s/n): ");
            char yn[4]; fgets(yn, sizeof(yn), stdin);
            if (tolower((unsigned char)yn[0]) == 's')
                guardar_examen(lista, nombre);
        }

    } while (op != 's');

    destruir_lista(lista);
}

//Para aplicar el examen
void flujo_aplicar(void)
{
    limpiar_pantalla();
    printf("\n  ===== APLICAR EXAMEN =====\n");
    listar_examenes();
    printf("  Nombre del examen a aplicar (sin .txt): ");

    char nombre[MAX_NOMBRE];
    fgets(nombre, sizeof(nombre), stdin);
    nombre[strcspn(nombre, "\r\n")] = '\0';
    if (strlen(nombre) == 0) return;

    ListaReactivos *lista = crear_lista();
    if (!cargar_examen(lista, nombre)) {
        pausar();
        destruir_lista(lista);
        return;
    }

    /* Calcular total de puntos posibles */
    float total_puntos = 0.0f;
    {
        Reactivo *cur = lista->cabeza;
        while (cur) { total_puntos += cur->puntos; cur = cur->siguiente; }
    }

    lista->actual = lista->cabeza;
    int num = 1;
    char buf[8];
    int finalizar = 0;

    do {
        limpiar_pantalla();
        mostrar_reactivo_quiz(lista, num, lista->total);

        fgets(buf, sizeof(buf), stdin);
        char op = (char)tolower((unsigned char)buf[0]);

        if (op == 'a') {
            if (lista->actual->anterior) { lista->actual = lista->actual->anterior; num--; }
            else { printf("  Estas en la primera pregunta.\n"); pausar(); }

        } else if (op == 'd') {
            if (lista->actual->siguiente) { lista->actual = lista->actual->siguiente; num++; }
            else { printf("  Estas en la ultima pregunta.\n"); pausar(); }

        } else if (op >= '1' && op <= '4') {
            lista->actual->respuesta_usuario = op - '0';

        } else if (op == 'f') {
            /* Confirmar finalización */
            printf("\n  Finalizar el examen? (s/n): ");
            fgets(buf, sizeof(buf), stdin);
            if (tolower((unsigned char)buf[0]) == 's')
                finalizar = 1;
        }

    } while (!finalizar);

    //Calificacion 
    limpiar_pantalla();
    printf("    RESULTADOS DEL EXAMEN \n\n");

    float puntos_logrados = 0.0f;
    int   num_r = 0;
    Reactivo *cur = lista->cabeza;

    while (cur) {
        num_r++;
        int respondio   = cur->respuesta_usuario;
        int es_correcta = (respondio == cur->correcta);

        printf("  [%d] %s\n", num_r, cur->pregunta);
        if (respondio)
            printf("      Tu respuesta: op%d (%s)  →  %s\n",
                   respondio,
                   cur->opcion[respondio - 1],
                   es_correcta ? "CORRECTA ✓" : "INCORRECTA ✗");
        else
            printf("      Sin respuesta  →  INCORRECTA ✗\n");

        printf("      Respuesta correcta: op%d (%s)  |  Puntos: %.1f\n\n",
               cur->correcta,
               cur->opcion[cur->correcta - 1],
               cur->puntos);

        if (es_correcta)
            puntos_logrados += cur->puntos;

        cur = cur->siguiente;
    }

    printf("  -----------------------------------------\n");
    printf("  PUNTAJE: %.1f / %.1f puntos\n", puntos_logrados, total_puntos);
    if (total_puntos > 0)
        printf("  PORCENTAJE: %.1f%%\n", (puntos_logrados / total_puntos) * 100.0f);
    printf("  =========================================\n");

    pausar();
    destruir_lista(lista);
}

//MENÚ PRINCIPAL
void menu_principal(void)
{
    char buf[8];
    int  op;

    do {
        limpiar_pantalla();
        printf("\n");
        printf("           GENERADOR DE EXAMENES            \n");
        printf("  ------------------------------------------\n");
        printf("   Generar un examen......1\n");
        printf("   Modificar un examen....2\n");
        printf("   Aplicar un examen......3\n");
        printf("   Salir..................4\n");
        printf("   Elige una opcion: ");

        fgets(buf, sizeof(buf), stdin);
        op = atoi(buf);

        switch (op) {
            case 1: flujo_generar();    break;
            case 2: flujo_modificar();  break;
            case 3: flujo_aplicar();    break;
            case 4: printf("\n  Hasta luego!\n\n"); break;
            default:
                printf("  Opcion no valida.\n");
                pausar();
        }
    } while (op != 4);
}

//main
int main(void)
{
    menu_principal();
    return 0;
}