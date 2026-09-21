import pandas as pd
import matplotlib.pyplot as plt

def load_data(path, version_label):
    """Carica un file CSV e aggiunge l'etichetta della versione."""
    df = pd.read_csv(path, sep=",", header="infer")
    df['version'] = version_label
    return df

def load_all_data():
    """Carica e unifica tutti i dataset con le rispettive etichette."""
    df_scalar   = load_data('./parallel/results_parallel.data', 'Scalare')
    df_simd     = load_data('./parallel/results_parallel_simd.data', 'SIMD')
    df_soa      = load_data('./parallel/results_parallel_soa.data', 'SIMD/SoA')

    return pd.concat([df_scalar, df_simd, df_soa], ignore_index=True)

def load_sequential_data(filepath_or_buffer):
    """Carica i dati del benchmark sequenziale."""
    df_seq = pd.read_csv(filepath_or_buffer, skipinitialspace=True)
    # Rinomina per evitare collisioni nei merge
    df_seq = df_seq.rename(columns={'time': 'seq_time'})
    return df_seq

def plot_speedup_all_vs_sequential(df_parallel, df_seq, metric='total_time', show_values=True):
    """
    Calcola e plotta lo Speedup di ogni versione parallela (Scalare, SIMD, SIMD/SoA)
    rispetto alla versione Sequenziale, suddiviso per tipologia di network:
    Speedup = Tempo_Sequenziale / Tempo_Parallelo
    """
    # 1. Unione sui soli valori di 'bodies' presenti in entrambi i dataset
    merged = pd.merge(df_parallel, df_seq, on='bodies', how='inner')

    if merged.empty:
        print("Attenzione: nessun valore di 'bodies' combacia tra sequenziale e parallelo!")
        return

    # 2. Calcolo Speedup puntuale
    merged['speedup_vs_seq'] = merged['seq_time'] / merged[metric]

    # Stili per distinguere chiaramente le versioni
    styles = {
        'Scalare':  {'color': '#1f77b4', 'marker': 'o', 'linestyle': '-'},
        'SIMD':     {'color': '#ff7f0e', 'marker': 's', 'linestyle': '--'},
        'SIMD/SoA': {'color': '#2ca02c', 'marker': '^', 'linestyle': '-.'}
    }

    networks = merged['network'].unique()
    fig, axes = plt.subplots(1, len(networks), figsize=(7 * len(networks), 6), sharey=True)
    if len(networks) == 1:
        axes = [axes]

    for ax, net in zip(axes, networks):
        subset_net = merged[merged['network'] == net]

        for ver in ['Scalare', 'SIMD', 'SIMD/SoA']:
            subset = subset_net[subset_net['version'] == ver].sort_values(by='bodies')
            if subset.empty:
                continue

            line = ax.plot(
                subset['bodies'],
                subset['speedup_vs_seq'],
                label=f'{ver}',
                linewidth=2,
                markersize=7,
                **styles.get(ver, {})
            )

            # 3. Annotazione opzionale con il valore numerico esatto sul marker
            if show_values:
                for _, row in subset.iterrows():
                    val = row['speedup_vs_seq']
                    ax.annotate(
                        f'{val:.1f}x',
                        (row['bodies'], val),
                        textcoords="offset points",
                        xytext=(0, 7),
                        ha='center',
                        fontsize=8,
                        fontweight='bold',
                        color=styles.get(ver, {}).get('color', 'black')
                    )

        # Baseline sequenziale = 1.0x
        ax.axhline(1.0, color='gray', linestyle=':', linewidth=1.5, label='Baseline (Seq = 1.0x)')

        # Linea ideale (Speedup teorico = numero di processi) se 'processors' è definito
        if 'processors' in subset_net.columns:
            n_proc = subset_net['processors'].iloc[0]
            ax.axhline(n_proc, color='red', linestyle='--', alpha=0.6, label=f'Ideale Lineare ({n_proc}x)')

        ax.set_title(f'Rete: {net}', fontsize=13, fontweight='bold', pad=12)
        ax.set_xlabel('Numero di Bodies', fontsize=11)
        ax.set_xticks(subset_net['bodies'].unique())
        ax.grid(True, linestyle='--', alpha=0.6)
        ax.legend(title='Implementazione', fontsize=9)

    axes[0].set_ylabel(f'Speedup su {metric} (x volte)', fontsize=11)
    plt.suptitle(f'Speedup delle Versioni Parallele rispetto al Sequenziale ({metric})', fontsize=14, y=0.98)
    plt.tight_layout(rect=[0, 0, 1, 0.92])
    plt.show()

    # 4. Stampa a console della tabella riassuntiva dei valori numerici
    summary_table = merged.pivot_table(
        index=['network', 'bodies'],
        columns='version',
        values='speedup_vs_seq'
    ).round(2)

    print("\n--- Tabella Speedup vs Sequenziale (x volte) ---")
    print(summary_table)

def plot_version_comparison(df, metric='total_time'):
    networks = df['network'].unique()
    fig, axes = plt.subplots(1, len(networks), figsize=(6 * len(networks), 5), sharey=True)
    if len(networks) == 1:
        axes = [axes]

    styles = {
        'Scalare':  {'color': '#1f77b4', 'marker': 'o', 'linestyle': '-'},
        'SIMD':     {'color': '#ff7f0e', 'marker': 's', 'linestyle': '--'},
        'SIMD/SoA': {'color': '#2ca02c', 'marker': '^', 'linestyle': '-.'}
    }

    metric_labels = {
        'total_time': 'Total Time (s)',
        'cpu_time':   'CPU Time (s)',
        'net_time':   'Network Time (s)'
    }

    for ax, net in zip(axes, networks):
        subset_net = df[df['network'] == net]
        for ver in ['Scalare', 'SIMD', 'SIMD/SoA']:
            subset = subset_net[subset_net['version'] == ver].sort_values(by='bodies')
            if not subset.empty:
                ax.plot(
                    subset['bodies'],
                    subset[metric],
                    label=ver,
                    **styles[ver],
                    linewidth=2,
                    markersize=7
                )

        # pad=12 aggiunge respiro sopra il singolo asse
        ax.set_title(f'Rete: {net}', fontsize=13, fontweight='bold', pad=12)
        ax.set_xlabel('Numero di Bodies', fontsize=11)
        ax.set_xticks(df['bodies'].unique())
        ax.grid(True, linestyle='--', alpha=0.6)
        ax.legend(title='Implementazione', fontsize=10)

    axes[0].set_ylabel(metric_labels.get(metric, metric), fontsize=11)

    # y=0.98 posiziona il titolo globale dentro i confini superiori
    plt.suptitle(f'Confronto Prestazioni: {metric_labels.get(metric, metric)}', fontsize=14, y=0.98)

    # rect=[left, bottom, right, top]: lascia il 10% di margine superiore per i titoli
    plt.tight_layout(rect=[0, 0, 1, 0.92])
    plt.show()


def plot_speedup_vs_scalar(df, metric='total_time'):
    pivot = df.pivot_table(index=['network', 'bodies'], columns='version', values=metric).reset_index()

    pivot['Speedup SIMD']     = pivot['Scalare'] / pivot['SIMD']
    pivot['Speedup SIMD/SoA'] = pivot['Scalare'] / pivot['SIMD/SoA']

    networks = pivot['network'].unique()
    fig, axes = plt.subplots(1, len(networks), figsize=(6 * len(networks), 5), sharey=True)
    if len(networks) == 1:
        axes = [axes]

    for ax, net in zip(axes, networks):
        subset = pivot[pivot['network'] == net].sort_values(by='bodies')

        ax.plot(subset['bodies'], subset['Speedup SIMD'], marker='s', label='Speedup SIMD', color='#ff7f0e', linewidth=2)
        ax.plot(subset['bodies'], subset['Speedup SIMD/SoA'], marker='^', label='Speedup SIMD/SoA', color='#2ca02c', linewidth=2)

        ax.axhline(1.0, color='gray', linestyle=':', linewidth=1.5, label='Baseline (Scalare = 1.0x)')

        ax.set_title(f'Rete: {net}', fontsize=13, fontweight='bold', pad=12)
        ax.set_xlabel('Numero di Bodies', fontsize=11)
        ax.set_xticks(pivot['bodies'].unique())
        ax.grid(True, linestyle='--', alpha=0.6)
        ax.legend(fontsize=10)

    axes[0].set_ylabel(f'Speedup su {metric} (x volte)', fontsize=11)
    plt.suptitle(f'Speedup Relativo alla Versione Scalare ({metric})', fontsize=14, y=0.98)

    # rect=[0, 0, 1, 0.92] protegge l'area superiore
    plt.tight_layout(rect=[0, 0, 1, 0.92])
    plt.show()

def main():
    # 1. Carica e unifica tutti i dati
    df_all = load_all_data()
    df_seq = load_sequential_data("./sequential/results.data")
    # 3. Plot dello Speedup rispetto al Sequenziale
    plot_speedup_all_vs_sequential(df_all, df_seq, metric='total_time', show_values=True)

    # Se nei file hai testato più configurazioni di 'processors',
    # puoi filtrare per un numero fisso di processi prima di plottare, ad esempio:
    # df_all = df_all[df_all['processors'] == 4]

    # 2. Confronto tempi assoluti (Total Time e CPU Time)
    print("Visualizzazione: Confronto Total Time...")
    # plot_version_comparison(df_all, metric='total_time')

    print("Visualizzazione: Confronto CPU Time...")
    # plot_version_comparison(df_all, metric='cpu_time')

    # 3. Confronto Speedup normalizzato rispetto a Scalare
    print("Visualizzazione: Speedup rispetto a Scalare...")
    # plot_speedup_vs_scalar(df_all, metric='total_time')
    # plot_speedup_vs_scalar(df_all, metric='cpu_time')

if __name__ == '__main__':
    main()