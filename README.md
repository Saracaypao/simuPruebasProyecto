# Escape the Grid

Escape the Grid es un juego de puzzle en una cuadrícula 2D compuesta por triángulos. El jugador guía un circulo celeste desde el punto de inicio hasta la salida verde, sorteando muros negros/grises y aprovechando cristales color cyan que reflejan los movimientos en horizontal o vertical. En la variante “Cuevas de Cristal”, cada cierto número de pasos el tablero o la posición de la meta pueden cambiar, sumando un desafío dinámico.

## Descripción general del juego

- **Jugador (circulo celeste)**: la pieza que controlas.  
- **Meta (circulo verde)**: el destino que debes alcanzar.  
- **Muros (triángulos negros/grises)**: casillas intransitables.  
- **Cristales (triángulos cyan)**: al atravesarlos, reflejan automáticamente el recorrido que hayas hecho a su izquierda hacia la derecha (o de arriba hacia abajo), creando movimientos extra que pueden afectar el tablero.

El reto consiste en encontrar la ruta más eficiente desde tu triángulo celeste hasta el botón verde, y adaptándote a los cambios de mapa que se producen tras un número determinado de movimientos.

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

1. **Pantalla de inicio**  
   - Pulsar **JUGAR** para comenzar el nivel actual.  
   - Pulsar **AUTOCOMPLETAR** para que el juego muestre automáticamente la solución óptima.  
   - Pulsar **REINICIAR** para volver al estado inicial del nivel.

2. **Desplazamiento manual**  
   - Mover la pieza celeste dando clic sobre una casilla adyacente libre.  
   - Alternativamente, usar **las flechas del teclado** (↑ ↓ ← →) para avanzar en la dirección deseada.

3. **Modo automático**  
   - Una vez en juego, pulsar **ENTER** o el botón **AUTOCOMPLETAR**.  
   - La ruta óptima aparece ilustrada paso a paso.

4. **Dinámica de cristales y cambios de mapa**  
   - Cada vez que pases sobre un cristal color cyan, tu camino de la izquierda se reflejará hacia la derecha (o de arriba hacia abajo).  
   - Tras un contador de movimientos visible en pantalla, el diseño del tablero o la posición de la meta verde puede alterarse, exigiendo un nuevo cálculo de ruta.

## Controles

- **Flechas del teclado** (↑ ↓ ← →): mover al jugador.  
- **Clic izquierdo** sobre triángulo adyacente: mover al jugador.  
- **ENTER**: iniciar la solución automática luego de haber seleccionado "jugar".  
- **R**: reiniciar el nivel actual.  
- **Botones en pantalla**:  
  - **JUGAR**: comenzar partida manual.  
  - **AUTOCOMPLETAR**: mostrar solución automática.  
  - **REINICIAR**: volver al inicio del nivel.
 
## GIFs

- **Juego normal**
- ![Juego](https://media.giphy.com/media/McJuiH1q4aQjDzNJlS/giphy.gif)
- **Juego normal y presionando la tecla Enter**  
- ![Juego+Enter](https://media.giphy.com/media/gpOtJrjBl01hYA9x9n/giphy.gif)
- **Juego autocompletado y presionando la opción de Reiniciar**
- ![Juego autocompletado+Reiniciar](https://media.giphy.com/media/ksCOxPhj2aioOwfPuO/giphy.gif)   


