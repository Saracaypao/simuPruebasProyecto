# Escape the Grid

Escape the Grid es un juego de puzzle en una cuadrícula 2D compuesta por triángulos. El jugador guía un circulo celeste desde el punto de inicio hasta la salida verde, sorteando muros negros/grises y aprovechando cristales color cyan que reflejan los movimientos en horizontal o vertical. En la variante “Cuevas de Cristal”, cada cierto número de pasos el tablero o la posición de la meta pueden cambiar, sumando un desafío dinámico.

## Descripción general del juego

- **Jugador (circulo celeste)**: la pieza que controlas.  
- **Meta (circulo verde)**: el destino que debes alcanzar.  
- **Muros (triángulos negros/grises)**: casillas intransitables.  
- **Cristales (triángulos cyan)**: al atravesarlos, reflejan automáticamente el recorrido que hayas hecho a su izquierda hacia la derecha (o de arriba hacia abajo), creando movimientos extra que pueden afectar el tablero.

El reto consiste en encontrar la ruta más eficiente desde tu triángulo celeste hasta el botón verde, y adaptándote a los cambios de mapa que se producen tras un número determinado de movimientos. Al completar el nivel, se muestra una pantalla de victoria celebratoria con estadísticas del juego y la opción de jugar nuevamente.

## Instrucciones de compilación y ejecución

No se requiere ninguna dependencia extra ni configuración compleja:

1. **Ubicar el ejecutable**  
   - Primero, abrir la carpeta donde se descargó el proyecto; a continuación, dentro de esa carpeta, localizar el archivo `EscapeTheGrid.exe`.
     
3. **Ejecutar la aplicación**  
   - **Doble clic** sobre el ejecutable `EscapeTheGrid.exe`.  
   - O bien abrir una terminal, situarse en la carpeta que contiene `EscapeTheGrid.exe` y escribir:
     ```bash
     ./EscapeTheGrid.exe
     ```
  Tras unos instantes, se abrirá la ventana principal con el tablero listo para jugar.

## Cómo jugar

1. **Pantalla de título**  
   - Al iniciar el juego, aparece una pantalla de título con efectos visuales y partículas.  
   - Hacer clic en **"CLICK TO CONTINUE"** para acceder al menú principal del juego.

2. **Pantalla de menú principal**  
   - Pulsar **PLAY** para comenzar el nivel actual en modo manual.  
   - Pulsar **AUTO-SOLVE** para que el juego muestre automáticamente la solución óptima.  
   - Pulsar **RESTART** para volver al estado inicial del nivel.

3. **Desplazamiento manual**  
   - Mover la pieza celeste dando clic sobre una casilla adyacente libre.  
   - Alternativamente, usar **las flechas del teclado** (↑ ↓ ← →) para avanzar en la dirección deseada.

4. **Modo automático**  
   - Una vez en juego, pulsar **ENTER** o el botón **AUTO-SOLVE**.  
   - La ruta óptima aparece ilustrada paso a paso.

5. **Pantalla de victoria**  
   - Al completar el nivel, aparece una pantalla de celebración con:
     - Trofeo dorado pixelado animado
     - Texto "YOU WIN!" con efectos pulsantes
     - Estadísticas del juego (tiempo y número de movimientos)
     - Partículas doradas celebratorias
     - Botón **"PLAY AGAIN"** para reiniciar y jugar de nuevo
   - La pantalla se adapta automáticamente a diferentes tamaños de ventana.

6. **Dinámica de cristales y cambios de mapa**  
   - Cada vez que pases sobre un cristal color cyan, tu camino de la izquierda se reflejará hacia la derecha (o de arriba hacia abajo).  
   - Tras un contador de movimientos visible en pantalla, el diseño del tablero o la posición de la meta verde puede alterarse, exigiendo un nuevo cálculo de ruta.

## Controles

- **Flechas del teclado** (↑ ↓ ← →): mover al jugador.  
- **Clic izquierdo** sobre triángulo adyacente: mover al jugador.  
- **ENTER**: iniciar la solución automática luego de haber seleccionado "play".  
- **R**: reiniciar el nivel actual.  
- **V**: activar instantáneamente la pantalla de victoria (tecla de debug para pruebas).
- **Botones en pantalla**:  
  - **PLAY**: comenzar partida manual.  
  - **AUTO-SOLVE**: mostrar solución automática.  
  - **RESTART**: volver al inicio del nivel.
  - **PLAY AGAIN**: en la pantalla de victoria, reiniciar el juego para una nueva partida.

## Características de la pantalla de victoria

La pantalla de victoria incluye múltiples elementos visuales y funcionales:

- **🏆 Trofeo pixelado**: Diseño retro con animaciones doradas que se adapta al tamaño de pantalla
- **✨ Efectos visuales**: Partículas doradas animadas que flotan por la pantalla
- **📊 Estadísticas del juego**: Muestra el tiempo total y número de movimientos realizados
- **🎮 Diseño responsive**: Se adapta automáticamente a diferentes resoluciones y tamaños de ventana
- **🔄 Funcionalidad de reinicio**: Botón "PLAY AGAIN" para comenzar una nueva partida inmediatamente
- **🎨 Animaciones suaves**: Texto pulsante y efectos de hover en los botones

La pantalla utiliza el mismo sistema responsive que la pantalla de título, garantizando una experiencia visual óptima en cualquier tamaño de ventana.
 
## GIFs

- **Juego normal**
1. ![Juego](https://media.giphy.com/media/McJuiH1q4aQjDzNJlS/giphy.gif)
- **Juego normal y presionando la tecla Enter**  
2. ![Juego+Enter](https://media.giphy.com/media/gpOtJrjBl01hYA9x9n/giphy.gif)
- **Juego autocompletado y presionando la opción de Reiniciar**
3. ![Juego autocompletado+Reiniciar](https://media.giphy.com/media/ksCOxPhj2aioOwfPuO/giphy.gif)   


