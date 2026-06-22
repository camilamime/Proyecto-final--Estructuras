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
    int  respuser;       /* 0 = sin responder        */

    struct Reactivo *anterior;
    struct Reactivo *siguiente;
} Reactivo;


typedef struct {
    Reactivo *ini;
    Reactivo *fin;
    Reactivo *act;
    int       total;
} ListaReactivos;

/* Lista */
ListaReactivos *crearlis(void);
void            destruirlis(ListaReactivos *lis);
Reactivo       *crear_reactivo(void);
void            insertaralfinal(ListaReactivos *lis, Reactivo *r);
void            eliminaract(ListaReactivos *lis);

/* Archivos */
void            lisrexamenes(void);
int             cargarexamen(ListaReactivos *lis, const char *nomexam);
void            guardarexamen(ListaReactivos *lis, const char *nomexam);

/* Menú y flujos */
void            menuprincipal(void);
void            flujogenerar(void);
void            flujomodificar(void);
void            flujoaplicar(void);

/* Edición de un reactivo */
void            editarreactivo(Reactivo *r);
void            mostrarreactivoedicion(ListaReactivos *lis, int num);
void            mostrarreactivoquiz(ListaReactivos *lis, int num, int total);

/* Utilidades */
void            limpiarpantalla(void);
void            pausar(void);
void            limpiarbuffer(void);
char            pedirnavegacion(void);

/* Lista doblemente enlazada*/

ListaReactivos *crearlis(void)
{
    ListaReactivos *l = (ListaReactivos *)malloc(sizeof(ListaReactivos));
    if (!l) { fprintf(stderr, "Error: sin memoria.\n"); exit(1); }
    l->ini = l->fin = l->act = NULL;
    l->total = 0;
    return l;
}

void destruirlis(ListaReactivos *lis)
{
    Reactivo *cur = lis->ini;
    while (cur) {
        Reactivo *sig = cur->siguiente;
        free(cur);
        cur = sig;
    }
    free(lis);
}

Reactivo *crear_reactivo(void)
{
    Reactivo *r = (Reactivo *)calloc(1, sizeof(Reactivo));
    if (!r) { fprintf(stderr, "Error: sin memoria.\n"); exit(1); }
    return r;
}

void insertaralfinal(ListaReactivos *lis, Reactivo *r)
{
    r->anterior  = lis->fin;
    r->siguiente = NULL;

    if (lis->fin)
        lis->fin->siguiente = r;
    else
        lis->ini = r;

    lis->fin = r;
    lis->total++;

    if (!lis->act)
        lis->act = r;
}

/* Elimina el nodo 'act' y avanza al siguiente (o al anterior) */
void eliminaract(ListaReactivos *lis)
{
    if (!lis->act) return;

    Reactivo *r    = lis->act;
    Reactivo *prev = r->anterior;
    Reactivo *next = r->siguiente;

    if (prev) prev->siguiente = next;
    else      lis->ini   = next;

    if (next) next->anterior = prev;
    else      lis->fin    = prev;

    lis->act = next ? next : prev;
    lis->total--;
    free(r);
}

/* ============================================================
   IMPLEMENTACIÓN – Archivos
   ============================================================ */

/* Lista los archivos .txt del directorio act que sean exámenes */
void lisrexamenes(void)
{
    printf("\n  Archivos de examen disponibles (.txt):\n");
    printf("  (Introduce el nomexam sin extension)\n\n");

    /* Usamos popen para lisr archivos .txt */
#ifdef _WIN32
    FILE *p = popen("dir /b *.txt 2>nul", "r");
#else
    FILE *p = popen("ls *.txt 2>/dev/null", "r");
#endif
    if (!p) {
        printf("  (No se pudieron lisr los archivos)\n");
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
int cargarexamen(ListaReactivos *lis, const char *nomexam)
{
    char arch[MAX_NOMBRE + 8];
    snprintf(arch, sizeof(arch), "%s.txt", nomexam);

    FILE *f = fopen(arch, "r");
    if (!f) {
        printf("  Error: no se pudo abrir '%s'.\n", arch);
        return 0;
    }

    char cad[MAX_TEXTO];
    Reactivo *r = NULL;

    while (fgets(cad, sizeof(cad), f)) {
        cad[strcspn(cad, "\r\n")] = '\0';

        if (strncmp(cad, ":p;", 3) == 0) {
            /* Nuevo reactivo */
            r = crear_reactivo();
            strncpy(r->pregunta, cad + 3, MAX_TEXTO - 1);

        } else if (strncmp(cad, ":op1;", 5) == 0 && r) {
            strncpy(r->opcion[0], cad + 5, MAX_OPCION - 1);

        } else if (strncmp(cad, ":op2;", 5) == 0 && r) {
            strncpy(r->opcion[1], cad + 5, MAX_OPCION - 1);

        } else if (strncmp(cad, ":op3;", 5) == 0 && r) {
            strncpy(r->opcion[2], cad + 5, MAX_OPCION - 1);

        } else if (strncmp(cad, ":op4;", 5) == 0 && r) {
            strncpy(r->opcion[3], cad + 5, MAX_OPCION - 1);

        } else if (strncmp(cad, ":r;", 3) == 0 && r) {
            /* :r;opN  →  N es el número de la opción correcta */
            char *ptr = cad + 3;          /* "opN" */
            if (strncmp(ptr, "op", 2) == 0)
                r->correcta = atoi(ptr + 2);

        } else if (r && strlen(cad) > 0 &&
                   (cad[strlen(cad)-1] == '.' ||
                    (cad[0] >= '0' && cad[0] <= '9'))) {
            /* Línea de puntos: "NN." */
            r->puntos = atof(cad);
            insertaralfinal(lis, r);
            r = NULL;
        }
    }
    /* Si quedó un reactivo sin línea de puntos (archivo mal terminado) */
    if (r) {
        r->puntos = 1.0f;
        insertaralfinal(lis, r);
    }

    fclose(f);
    printf("  Examen '%s' cargado: %d reactivo(s).\n", nomexam, lis->total);
    return 1;
}

//Guarda la lis en el archivo con el formato especificado
void guardarexamen(ListaReactivos *lis, const char *nomexam)
{
    char arch[MAX_NOMBRE + 8];
    snprintf(arch, sizeof(arch), "%s.txt", nomexam);

    FILE *f = fopen(arch, "w");
    if (!f) {
        printf("  Error: no se pudo guardar '%s'.\n", arch);
        return;
    }

    Reactivo *cur = lis->ini;
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
    printf("  Examen guardado en '%s' (%d reactivo(s)).\n", arch, lis->total);
}

//IMPLEMENTACIÓN – Utilidades de pantalla
void limpiarpantalla(void)
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
    limpiarbuffer();
    getchar();
}

void limpiarbuffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* Devuelve 'a'=anterior, 'd'=siguiente, 'q'=salir, 'n'=nuevo,
   'e'=eliminar, 's'=guardar, 'r'=responder */
char pedirnavegacion(void)
{
    char buf[8];
    printf("\n  [A]nterior  [D]erecha/Siguiente  [N]uevo  [E]liminar  [G]uardar  [S]alir\n  Opcion: ");
    fgets(buf, sizeof(buf), stdin);
    return (char)tolower((unsigned char)buf[0]);
}

  //EDICIÓN de un reactivo
void editarreactivo(Reactivo *r)
{
    char buf[MAX_TEXTO];

    printf("\n  --- Edicion del Reactivo ---\n");

    printf("  Pregunta [act: %.60s...]\n  > ", r->pregunta);
    fgets(buf, sizeof(buf), stdin);
    buf[strcspn(buf, "\r\n")] = '\0';
    if (strlen(buf) > 0) strncpy(r->pregunta, buf, MAX_TEXTO - 1);

    for (int i = 0; i < 4; i++) {
        printf("  Opcion %d [act: %s]\n  > ", i+1, r->opcion[i]);
        fgets(buf, sizeof(buf), stdin);
        buf[strcspn(buf, "\r\n")] = '\0';
        if (strlen(buf) > 0) strncpy(r->opcion[i], buf, MAX_OPCION - 1);
    }

    printf("  Respuesta correcta (1-4) [act: %d]\n  > ", r->correcta);
    fgets(buf, sizeof(buf), stdin);
    buf[strcspn(buf, "\r\n")] = '\0';
    int c = atoi(buf);
    if (c >= 1 && c <= 4) r->correcta = c;

    printf("  Puntos [act: %.1f]\n  > ", r->puntos);
    fgets(buf, sizeof(buf), stdin);
    buf[strcspn(buf, "\r\n")] = '\0';
    float p = atof(buf);
    if (p > 0) r->puntos = p;

    printf("  Reactivo actizado.\n");
}

/* Muestra el reactivo act en modo edición */
void mostrarreactivoedicion(ListaReactivos *lis, int num)
{
    Reactivo *r = lis->act;
    if (!r) { printf("  (Sin reactivos)\n"); return; }

    printf("\n  ----- Reactivo %d / %d -----\n", num, lis->total);
    printf("  P: %s\n\n", r->pregunta);
    for (int i = 0; i < 4; i++)
        printf("  op%d: %s\n", i+1, r->opcion[i]);
    printf("\n  Correcta: op%d  |  Puntos: %.1f\n", r->correcta, r->puntos);
}

/* Muestra el reactivo act en modo quiz */
void mostrarreactivoquiz(ListaReactivos *lis, int num, int total)
{
    Reactivo *r = lis->act;
    if (!r) return;

    printf("\n  ----- Pregunta %d de %d -----\n\n", num, total);
    printf("  %s\n\n", r->pregunta);
    for (int i = 0; i < 4; i++) {
        char marca = (r->respuser == i+1) ? '*' : ' ';
        printf("  %c [%d] %s\n", marca, i+1, r->opcion[i]);
    }
    if (r->respuser)
        printf("\n  (Tu respuesta: op%d) \n ", r->respuser);
    else
        printf("\n  (Sin respuesta)\n");

    printf("\n  [A]nterior  [D]erecha  [1-4] Responder  [F]inalizar\n");
}

   //FLUJOS PRINCIPALES
//Para generar el examen
void flujogenerar(void)
{
    limpiarpantalla();
    printf("\n  ----- GENERAR EXAMEN -----\n");
    printf("  Nombre del nuevo examen (sin .txt): ");

    char nomexam[MAX_NOMBRE];
    fgets(nomexam, sizeof(nomexam), stdin);
    nomexam[strcspn(nomexam, "\r\n")] = '\0';
    if (strlen(nomexam) == 0) return;

    ListaReactivos *lis = crearlis();
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
    insertaralfinal(lis, r);
    num = 1;

    editarreactivo(lis->act);

    do {
        limpiarpantalla();
        mostrarreactivoedicion(lis, num);

        op = pedirnavegacion();

        if (op == 'a') {
            if (lis->act->anterior) {
                lis->act = lis->act->anterior;
                num--;
            } else printf("  Ya estas en el primer reactivo.\n");

        } else if (op == 'd') {
            if (lis->act->siguiente) {
                lis->act = lis->act->siguiente;
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
            insertaralfinal(lis, nuevo);
            /* Ir al nuevo */
            lis->act = lis->fin;
            num = lis->total;
            limpiarpantalla();
            editarreactivo(lis->act);

        } else if (op == 'e') {
            if (lis->total == 1) {
                printf("  No puedes eliminar el unico reactivo.\n");
                pausar();
            } else {
                eliminaract(lis);
                num = (num > lis->total) ? lis->total : num;
                printf("  Reactivo eliminado.\n");
                pausar();
            }

        } else if (op == 'g') {
            guardarexamen(lis, nomexam);
            pausar();

        } else if (op == 's') {
            printf("  Guardar antes de salir? (s/n): ");
            char yn[4]; fgets(yn, sizeof(yn), stdin);
            if (tolower((unsigned char)yn[0]) == 's')
                guardarexamen(lis, nomexam);
        }

    } while (op != 's');

    destruirlis(lis);
}

//Modificar el examen
void flujomodificar(void)
{
    limpiarpantalla();
    printf("\n  ===== MODIFICAR EXAMEN =====\n");
    lisrexamenes();
    printf("  Nombre del examen a modificar (sin .txt): ");

    char nomexam[MAX_NOMBRE];
    fgets(nomexam, sizeof(nomexam), stdin);
    nomexam[strcspn(nomexam, "\r\n")] = '\0';
    if (strlen(nomexam) == 0) return;

    ListaReactivos *lis = crearlis();
    if (!cargarexamen(lis, nomexam)) {
        pausar();
        destruirlis(lis);
        return;
    }
    pausar();

    int  num = 1;
    char op;

    do {
        limpiarpantalla();
        mostrarreactivoedicion(lis, num);

        printf("\n  [A]nterior  [D]siguiente  [N]uevo  [E]ditar  [X]eliminar  [G]uardar  [S]alir\n  Opcion: ");
        char buf[8];
        fgets(buf, sizeof(buf), stdin);
        op = (char)tolower((unsigned char)buf[0]);

        if (op == 'a') {
            if (lis->act->anterior) { lis->act = lis->act->anterior; num--; }
            else { printf("  Primer reactivo.\n"); pausar(); }

        } else if (op == 'd') {
            if (lis->act->siguiente) { lis->act = lis->act->siguiente; num++; }
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
            insertaralfinal(lis, nuevo);
            lis->act = lis->fin;
            num = lis->total;
            limpiarpantalla();
            editarreactivo(lis->act);

        } else if (op == 'e') {
            limpiarpantalla();
            editarreactivo(lis->act);

        } else if (op == 'x') {
            if (lis->total == 1) { printf("  No puedes eliminar el unico reactivo.\n"); pausar(); }
            else {
                eliminaract(lis);
                if (num > lis->total) num = lis->total;
                printf("  Reactivo eliminado.\n"); pausar();
            }

        } else if (op == 'g') {
            guardarexamen(lis, nomexam);
            pausar();

        } else if (op == 's') {
            printf("  Guardar antes de salir? (s/n): ");
            char yn[4]; fgets(yn, sizeof(yn), stdin);
            if (tolower((unsigned char)yn[0]) == 's')
                guardarexamen(lis, nomexam);
        }

    } while (op != 's');

    destruirlis(lis);
}

//Para aplicar el examen
void flujoaplicar(void)
{
    limpiarpantalla();
    printf("\n  ===== APLICAR EXAMEN =====\n");
    lisrexamenes();
    printf("  Nombre del examen a aplicar (sin .txt): ");

    char nomexam[MAX_NOMBRE];
    fgets(nomexam, sizeof(nomexam), stdin);
    nomexam[strcspn(nomexam, "\r\n")] = '\0';
    if (strlen(nomexam) == 0) return;

    ListaReactivos *lis = crearlis();
    if (!cargarexamen(lis, nomexam)) {
        pausar();
        destruirlis(lis);
        return;
    }

    /* Calcular total de puntos posibles */
    float ptmax = 0.0f;
    {
        Reactivo *cur = lis->ini;
        while (cur) { ptmax += cur->puntos; cur = cur->siguiente; }
    }

    lis->act = lis->ini;
    int num = 1;
    char buf[8];
    int finalizar = 0;

    do {
        limpiarpantalla();
        mostrarreactivoquiz(lis, num, lis->total);

        fgets(buf, sizeof(buf), stdin);
        char op = (char)tolower((unsigned char)buf[0]);

        if (op == 'a') {
            if (lis->act->anterior) { lis->act = lis->act->anterior; num--; }
            else { printf("  Estas en la primera pregunta.\n"); pausar(); }

        } else if (op == 'd') {
            if (lis->act->siguiente) { lis->act = lis->act->siguiente; num++; }
            else { printf("  Estas en la ultima pregunta.\n"); pausar(); }

        } else if (op >= '1' && op <= '4') {
            lis->act->respuser = op - '0';

        } else if (op == 'f') {
            /* Confirmar finalización */
            printf("\n  Finalizar el examen? (s/n): ");
            fgets(buf, sizeof(buf), stdin);
            if (tolower((unsigned char)buf[0]) == 's')
                finalizar = 1;
        }

    } while (!finalizar);

    //Calificacion 
    limpiarpantalla();
    printf("    RESULTADOS DEL EXAMEN \n\n");

    float ptob = 0.0f;
    int   numr = 0;
    Reactivo *cur = lis->ini;

    while (cur) {
        numr++;
        int resp   = cur->respuser;
        int acerto = (resp == cur->correcta);

        printf("  [%d] %s\n", numr, cur->pregunta);
        if (resp)
            printf("      Tu respuesta: op%d (%s)  ->  %s\n",
                   resp,
                   cur->opcion[resp - 1],
                   acerto ? "CORRECTA " : "INCORRECTA :p");
        else
            printf("      Sin respuesta  →  INCORRECTA  :P\n");

        printf("      Respuesta correcta: op%d (%s)  |  Puntos: %.1f\n\n",
               cur->correcta,
               cur->opcion[cur->correcta - 1],
               cur->puntos);

        if (acerto)
            ptob += cur->puntos;

        cur = cur->siguiente;
    }

    printf("  -----------------------------------------\n");
    printf("  PUNTAJE: %.1f / %.1f puntos\n", ptob, ptmax);
    if (ptmax > 0)
        printf("  PORCENTAJE: %.1f%%\n", (ptob / ptmax) * 100.0f);
    printf("  =========================================\n");

    pausar();
    destruirlis(lis);
}

//MENÚ PRINCIPAL
void menuprincipal(void)
{
    char buf[8];
    int  op;

    do {
        limpiarpantalla();
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
            case 1: flujogenerar();    break;
            case 2: flujomodificar();  break;
            case 3: flujoaplicar();    break;
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
    menuprincipal();
    return 0;
}