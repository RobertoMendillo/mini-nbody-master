import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# 1. Carica i dati generati dal programma C
df = pd.read_csv("simulation_data.csv")

# 2. Configura il grafico 3D
fig = plt.figure(figsize=(8, 8))
ax = fig.add_subplot(111, projection='3d')

# Imposta i limiti dello spazio basandoti sulla tua inizializzazione (es. da -1 a 1)
ax.set_xlim([-1.5, 1.5])
ax.set_ylim([-1.5, 1.5])
ax.set_zlim([-1.5, 1.5])

# Crea l'oggetto grafico per le particelle (punti nello spazio)
scatter = ax.scatter([], [], [], c='blue', s=2)

# 3. Funzione di aggiornamento per l'animazione (eseguita per ogni frame)
def update(frame):
    # Filtra i dati per l'iterazione corrente
    current_data = df[df['iteration'] == frame]
    
    # Aggiorna le posizioni x, y, z nel grafico
    scatter._offsets3d = (current_data['x'], current_data['y'], current_data['z'])
    ax.set_title(f"N-Body Simulation - Iterazione {frame}")
    return scatter,

# 4. Avvia l'animazione
num_frames = df['iteration'].max()
ani = FuncAnimation(fig, update, frames=range(1, num_frames + 1), interval=500, blit=False)

plt.show()