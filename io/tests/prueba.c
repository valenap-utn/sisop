#include <cspecs/cspec.h>
#include <string.h>
#include <stdio.h>

context (probando_cosas) {
    describe("tests") {
        before {
            printf("Yo inicializo cosas\n");
        } end

        after {
            printf("Yo limpio cosas\n");
        } end

        it("test1") {
            printf("Soy el test 1 y pruebo que 1+1 sea 2\n");
            should_int(1 + 1) be equal to(2);
        } end

        // it("test2") {
        //     printf("Soy el test 2 y doy Segmentation Fault\n");
        //     char* puntero = NULL;
        //     *puntero = 9;
        // } end

        it("test3") {
            printf("Soy el test 3");
        } end
    } end
}

/*
    El bloque before se ejecuta antes de cada test dentro de la misma suite.
    El bloque after se ejecuta después de cada test dentro de la misma suite.
    Si un test rompe (tira segmentation fault) aborta la ejecución de los demás.
    Los tests corren en orden, aunque tu código nunca debería confiar en esto. 

    Los tests tienen que ser independientes entre sí, y cualquier efecto de lado 
    que tengan DEBE SER LIMPIADO en el bloque after y reinicializado en el before 
    para que el siguiente test tenga el mismo contexto de ejecución.

    https://docs.utnso.com.ar/guias/herramientas/cspec#ejemplo-1-orden-inicializacion-y-limpieza
    https://github.com/mumuki/cspec/blob/master/README_ES.md#should
*/
