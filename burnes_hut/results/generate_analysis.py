import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# 1. Definizione intestazioni corrette
headers_seq = ['bodies', 'total_time', 'cache_miss_L1', 'cache_miss_L2']
headers_par = ['network', 'bodies', 'cpu_time', 'net_time', 'cache_miss_L1', 'cache_miss_L2']

# 2. Caricamento Dati
df_seq = pd.read_csv('../../results/results.data')
df_par = pd.read_csv('results_parallel.data', skiprows=1, names=headers_par, skipinitialspace=True)
df_soa = pd.read_csv('results_parallel_soa.data', skiprows=1, names=headers_par, skipinitialspace=True)

df_par['network'] = df_par['network'].str.strip()
df_soa['network'] = df_soa['network'].str.strip()

# Calcolo del tempo totale per i dataset paralleli
df_par['total_time'] = df_par['cpu_time'] + df_par['net_time']
df_soa['total_time'] = df_soa['cpu_time'] + df_soa['net_time']

df_par = df_par[df_par['total_time'] > 0].copy()
df_soa = df_soa[df_soa['total_time'] > 0].copy()

df_seq_mean = df_seq.groupby('bodies')['total_time'].mean().reset_index().rename(columns={'total_time': 't_seq'})


def compute_speedup(df_parallel, df_sequential, network_name):
    filtered = df_parallel[df_parallel['network'] == network_name].copy()
    mean_par = filtered.groupby('bodies')[['total_time', 'net_time', 'cpu_time']].mean().reset_index()
    merged = pd.merge(mean_par, df_sequential[['bodies', 't_seq']], on='bodies')
    merged['speedup'] = merged['t_seq'] / merged['total_time']
    merged['comp_time'] = merged['cpu_time']  # comp_time corrisponde al cpu_time
    merged['net_ratio_%'] = (merged['net_time'] / merged['total_time']) * 100
    return merged


# Calcolo per Native InfiniBand
speedup_aos_native = compute_speedup(df_par, df_seq_mean, 'native/infiniband')
speedup_soa_native = compute_speedup(df_soa, df_seq_mean, 'native/infiniband')

# Tabella Native InfiniBand
summary_native = pd.merge(
    speedup_aos_native[['bodies', 't_seq', 'total_time', 'speedup']],
    speedup_soa_native[['bodies', 'total_time', 'speedup']],
    on='bodies',
    suffixes=('_AoS', '_SoA'),
    how='outer'
).sort_values('bodies')
summary_native.columns = ['Bodies (N)', 'T_Seq (s)', 'T_AoS (s)', 'Speedup AoS', 'T_SoA (s)', 'Speedup SoA']
summary_native['Speedup SoA/AoS'] = summary_native['T_AoS (s)'] / summary_native['T_SoA (s)']

# --- GRAFICO 1: TEMPI ED ESECUZIONE + SPEEDUP (NATIVE INFINIBAND) ---
plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

ax1.plot(df_seq_mean['bodies'], df_seq_mean['t_seq'], 'o:', color='#1f77b4', linewidth=2.2, markersize=6,
         label='Sequenziale Base')
ax1.plot(speedup_aos_native['bodies'], speedup_aos_native['total_time'], 's-', color='#ff7f0e', linewidth=2.2,
         markersize=6, label='Parallelo (AoS)')
ax1.plot(speedup_soa_native['bodies'], speedup_soa_native['total_time'], '^-', color='#2ca02c', linewidth=2.2,
         markersize=6, label='Parallelo (SoA)')

ax1.set_yscale('log')
ax1.set_title('Tempo Totale di Esecuzione (Native InfiniBand)', fontsize=13, fontweight='bold', pad=22)
ax1.set_xlabel('Numero di Corpi (N)', fontsize=11)
ax1.set_ylabel('Tempo di Esecuzione [s] (scala logaritmica)', fontsize=11)
ax1.grid(True, which='both', linestyle='--', alpha=0.5)
ax1.legend(fontsize=10, loc='upper left')

ax2.axhline(1.0, color='gray', linestyle='--', linewidth=1.5, label='Baseline Sequenziale (1.0x)')
ax2.plot(speedup_aos_native['bodies'], speedup_aos_native['speedup'], 's-', color='#ff7f0e', linewidth=2.5,
         markersize=7, label='Speedup AoS')
ax2.plot(speedup_soa_native['bodies'], speedup_soa_native['speedup'], '^-', color='#2ca02c', linewidth=2.5,
         markersize=7, label='Speedup SoA')

max_speedup_val = max(speedup_aos_native['speedup'].max(), speedup_soa_native['speedup'].max())
ax2.set_ylim(0, max_speedup_val * 1.30)

max_aos = speedup_aos_native.loc[speedup_aos_native['speedup'].idxmax()]
max_soa = speedup_soa_native.loc[speedup_soa_native['speedup'].idxmax()]

ax2.scatter([max_aos['bodies']], [max_aos['speedup']], color='#d62728', s=75, zorder=5)
ax2.scatter([max_soa['bodies']], [max_soa['speedup']], color='#d62728', s=75, zorder=5)

ax2.annotate(f"Max SoA: {max_soa['speedup']:.2f}x\n(N={int(max_soa['bodies']):,})",
             xy=(max_soa['bodies'], max_soa['speedup']),
             xytext=(-140, 20),
             textcoords="offset points",
             arrowprops=dict(arrowstyle='->', color='#2ca02c', lw=1.6),
             fontsize=9.5, fontweight='semibold',
             bbox=dict(boxstyle='round,pad=0.4', facecolor='#eafaf1', edgecolor='#2ca02c', alpha=0.95))

ax2.annotate(f"Max AoS: {max_aos['speedup']:.2f}x\n(N={int(max_aos['bodies']):,})",
             xy=(max_aos['bodies'], max_aos['speedup']),
             xytext=(-140, -48),
             textcoords="offset points",
             arrowprops=dict(arrowstyle='->', color='#ff7f0e', lw=1.6),
             fontsize=9.5, fontweight='semibold',
             bbox=dict(boxstyle='round,pad=0.4', facecolor='#fff3e6', edgecolor='#ff7f0e', alpha=0.95))

ax2.set_title('Speedup vs Sequenziale (Native InfiniBand)', fontsize=13, fontweight='bold', pad=22)
ax2.set_xlabel('Numero di Corpi (N)', fontsize=11)
ax2.set_ylabel('Speedup (Sp = T_seq / T_par)', fontsize=11)
ax2.grid(True, linestyle='--', alpha=0.5)
ax2.legend(fontsize=10, loc='lower right')

plt.tight_layout()
plt.savefig('speedup_native_infiniband.png', dpi=300)
plt.close()

# --- GRAFICO 2: CONFRONTO RETI ---
networks = ['native/infiniband', 'tcpip/infiniband', 'tcpip/ethernet']
labels = ['Native InfiniBand', 'TCP/IP InfiniBand', 'TCP/IP Ethernet (1GbE)']
colors = ['#2ca02c', '#1f77b4', '#d62728']
markers = ['^', 's', 'o']

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

for ax, mode_name, df_data in zip([ax1, ax2], ['AoS (Array of Structures)', 'SoA (Structure of Arrays)'],
                                  [df_par, df_soa]):
    ax.axhline(1.0, color='gray', linestyle='--', linewidth=1.5, label='Baseline Sequenziale (1.0x)')
    for net, label, col, mk in zip(networks, labels, colors, markers):
        res = compute_speedup(df_data, df_seq_mean, net)
        if not res.empty:
            ax.plot(res['bodies'], res['speedup'], marker=mk, linewidth=2.2, markersize=6, color=col, label=label)

    ax.set_title(f'Speedup per Rete - {mode_name}', fontsize=13, fontweight='bold', pad=18)
    ax.set_xlabel('Numero di Corpi (N)', fontsize=11)
    ax.set_ylabel('Speedup (Sp = T_seq / T_par)', fontsize=11)
    ax.grid(True, linestyle='--', alpha=0.5)
    ax.legend(fontsize=10, loc='center left' if 'AoS' in mode_name else 'upper left')

plt.tight_layout()
plt.savefig('speedup_networks_comparison.png', dpi=300)
plt.close()

# --- GRAFICO 3: SoA vs AoS e OVERHEAD COMUNICAZIONE ---
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

valid_comp = summary_native.dropna(subset=['Speedup SoA/AoS']).copy()
ax1.axhline(1.0, color='gray', linestyle='--', linewidth=1.5, label='Parità di Prestazioni (1.0x)')
ax1.plot(valid_comp['Bodies (N)'], valid_comp['Speedup SoA/AoS'], 'D-', color='#9467bd', linewidth=2.5, markersize=7,
         label='Rapporto T_AoS / T_SoA')

ax1.fill_between(valid_comp['Bodies (N)'], 1.0, valid_comp['Speedup SoA/AoS'],
                 where=(valid_comp['Speedup SoA/AoS'] >= 1.0), color='#2ca02c', alpha=0.2,
                 label='Regione SoA più efficiente')
ax1.fill_between(valid_comp['Bodies (N)'], 1.0, valid_comp['Speedup SoA/AoS'],
                 where=(valid_comp['Speedup SoA/AoS'] < 1.0), color='#ff7f0e', alpha=0.2,
                 label='Regione AoS più efficiente')

ax1.set_title('Speedup Relativo: SoA vs AoS (Native InfiniBand)', fontsize=13, fontweight='bold', pad=18)
ax1.set_xlabel('Numero di Corpi (N)', fontsize=11)
ax1.set_ylabel('Rapporto di Speedup (T_AoS / T_SoA)', fontsize=11)
ax1.grid(True, linestyle='--', alpha=0.5)
ax1.legend(fontsize=10, loc='lower right')

for net, label, col, mk in zip(networks, labels, colors, markers):
    res_net = compute_speedup(df_par, df_seq_mean, net)
    if not res_net.empty:
        ax2.plot(res_net['bodies'], res_net['net_ratio_%'], marker=mk, linewidth=2.2, markersize=6, color=col,
                 label=label)

ax2.set_title('Incidenza della Comunicazione di Rete (% su Tempo Totale)', fontsize=13, fontweight='bold', pad=18)
ax2.set_xlabel('Numero di Corpi (N)', fontsize=11)
ax2.set_ylabel('% Tempo Comunicazione (T_net / T_total * 100)', fontsize=11)
ax2.grid(True, linestyle='--', alpha=0.5)
ax2.legend(fontsize=10, loc='center right')

plt.tight_layout()
plt.savefig('soa_vs_aos_and_network_overhead.png', dpi=300)
plt.close()

# --- GRAFICO 4: VERDETTO TRA LE 3 RETI ---
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

net_names = ['Native InfiniBand\n(RDMA / Verbs)', 'TCP/IP InfiniBand\n(IP-over-IB)', 'TCP/IP Ethernet\n(1GbE)']
colors_palette = ['#2ca02c', '#1f77b4', '#d62728']

max_aos_vals = [
    compute_speedup(df_par, df_seq_mean, 'native/infiniband')['speedup'].max(),
    compute_speedup(df_par, df_seq_mean, 'tcpip/infiniband')['speedup'].max(),
    compute_speedup(df_par, df_seq_mean, 'tcpip/ethernet')['speedup'].max()
]
max_soa_vals = [
    compute_speedup(df_soa, df_seq_mean, 'native/infiniband')['speedup'].max(),
    compute_speedup(df_soa, df_seq_mean, 'tcpip/infiniband')['speedup'].max(),
    compute_speedup(df_soa, df_seq_mean, 'tcpip/ethernet')['speedup'].max()
]

x_pos = np.arange(len(net_names))
bar_w = 0.35

r1 = ax1.bar(x_pos - bar_w / 2, max_aos_vals, bar_w, label='AoS (Array of Structures)', color='#ff7f0e',
             edgecolor='#333333', linewidth=0.8)
r2 = ax1.bar(x_pos + bar_w / 2, max_soa_vals, bar_w, label='SoA (Structure of Arrays)', color='#2ca02c',
             edgecolor='#333333', linewidth=0.8)
ax1.axhline(1.0, color='#d62728', linestyle='--', linewidth=1.5, label='Baseline Sequenziale (1.0x)')

ax1.set_ylabel('Speedup Massimo Raggiunto', fontsize=12)
ax1.set_title('Verdetto: Picco Massimo di Speedup per Rete', fontsize=13, fontweight='bold', pad=18)
ax1.set_xticks(x_pos)
ax1.set_xticklabels(net_names, fontsize=10.5)
ax1.set_ylim(0, max(max_aos_vals) + 5)
ax1.grid(True, linestyle='--', alpha=0.5, axis='y')
ax1.legend(fontsize=10, loc='upper left')

for r in r1:
    h = r.get_height()
    ax1.annotate(f'{h:.2f}x', xy=(r.get_x() + r.get_width() / 2, h), xytext=(0, 4),
                 textcoords="offset points", ha='center', va='bottom', fontsize=10, fontweight='bold')
for r in r2:
    h = r.get_height()
    ax1.annotate(f'{h:.2f}x', xy=(r.get_x() + r.get_width() / 2, h), xytext=(0, 4),
                 textcoords="offset points", ha='center', va='bottom', fontsize=10, fontweight='bold')

avg_overhead = [
    compute_speedup(df_par, df_seq_mean, 'native/infiniband')['net_ratio_%'].mean(),
    compute_speedup(df_par, df_seq_mean, 'tcpip/infiniband')['net_ratio_%'].mean(),
    compute_speedup(df_par, df_seq_mean, 'tcpip/ethernet')['net_ratio_%'].mean()
]

b_ov = ax2.bar(net_names, avg_overhead, width=0.45, color=colors_palette, edgecolor='#333333', linewidth=0.8)
ax2.set_ylabel('% Media Tempo di Rete su Tempo Totale', fontsize=12)
ax2.set_title('Overhead Medio della Comunicazione di Rete', fontsize=13, fontweight='bold', pad=18)
ax2.set_ylim(0, max(avg_overhead) + 5)
ax2.grid(True, linestyle='--', alpha=0.5, axis='y')

for b in b_ov:
    h = b.get_height()
    ax2.annotate(f'{h:.1f}%', xy=(b.get_x() + b.get_width() / 2, h), xytext=(0, 4),
                 textcoords="offset points", ha='center', va='bottom', fontsize=10.5, fontweight='bold')

plt.tight_layout()
plt.savefig('confronto_reti_verdetto.png', dpi=300)
plt.close()

print('Script completato: tutti i grafici e confronti generati!')