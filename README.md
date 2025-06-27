# Escape the Grid

Escape the Grid es un juego de puzzle sobre una cuadrícula 2D en el que el jugador debe guiar a su avatar desde el punto de inicio hasta la meta, sorteando obstáculos y aprovechando mecánicas especiales. En la variante “Cuevas de Cristal”, el tablero incorpora cristales reflectantes que duplican tu ruta en horizontal o vertical, y cada cierto número de movimientos el mapa o la posición de la salida pueden alterarse, añadiendo un reto dinámico.

## Descripción general del juego

Al arrancar, el jugador se encuentra ante un tablero formado por celdas que pueden contener:

- **S (Start)**: punto de partida.  
- **G (Goal)**: meta que hay que alcanzar.  
- **# (Wall)**: obstáculos infranqueables.  
- **K (Key)**: elementos opcionales que suman puntos si se recogen.  
- **C (Crystal)**: al atravesar su casilla, el camino recorrido a la izquierda se “refleja” automáticamente hacia la derecha (o de arriba hacia abajo), creando copias de desplazamientos que pueden abrir o bloquear rutas.

El objetivo principal es llevar la pieza desde **S** hasta **G** con el menor número de pasos posible, explorando manualmente o utilizando el modo de resolución automática. La presencia de cristales y el posible “rebalanceo” del mapa tras ciertos turnos obliga a replantear la estrategia y a adaptarse a cambios inesperados.

## Instrucciones de compilación y ejecución

No se requiere ninguna dependencia extra ni configuración compleja:

1. **Ubicar el ejecutable**  
   - En **Windows**, encontrar `EscapeTheGrid.exe` en el Escritorio o en la carpeta de descargas.  
   - En **Linux/macOS** (si la extensión cambia) ubicar el binario equivalente.

2. **Ejecutar la aplicación**  
   - **Doble clic** sobre el icono en el Escritorio.  
   - O bien abrir una terminal, situarse en la carpeta que contiene `EscapeTheGrid.exe` y escribir:
     ```bash
     ./EscapeTheGrid.exe
     ```
   Tras unos segundos de carga, aparecerá la ventana principal con el tablero listo para jugar.

## Cómo jugar

1. **Selección de nivel**  
   - Al iniciarse, el juego muestra un menú para elegir un puzzle predefinido o cargar un archivo de mapa personalizado.  
2. **Modo manual**  
   - El jugador hace **clic** en una casilla adyacente libre para mover la pieza.  
   - Cada paso actualiza la posición y, si se atraviesa un cristal, se reflejarán movimientos extra.  
3. **Modo automático**  
   - Pulsar **“Resolver”** para que el programa aplique el algoritmo elegido (BFS, A*, etc.).  
   - La secuencia óptima se despliega paso a paso, con opción de pausar o avanzar manualmente.  
4. **Eventos dinámicos**  
   - Tras un número de turnos que se muestra en pantalla, el mapa puede “rotar” o cambiar la posición de **G**, obligando a recalcular la ruta sobre la marcha.

## Controles

- **Clic izquierdo sobre celda adyacente**: mover al avatar.  
- **Botón “Resolver”**: iniciar la búsqueda automática de la ruta más corta.  
- **Botón “Siguiente paso”**: avanzar un paso en la solución automática.  
- **Botón “Reiniciar”**: volver al estado inicial del nivel o recargar un nuevo mapa.  

> *Nota:* todos los comandos se realizan con el ratón; no hay atajos de teclado por defecto.

## Capturas de pantalla (recomendado)

Para enriquecer la documentación, incluir imágenes o GIFs ilustrativos:

1. **Tablero inicial** con todos los elementos (S, G, paredes, cristales).  
2. **Movimiento manual**, mostrando un ejemplo de clic y desplazamiento.  
3. **Solución automática** paso a paso, idealmente en formato GIF.  
