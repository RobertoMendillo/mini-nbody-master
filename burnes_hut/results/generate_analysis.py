from pathlib import Path
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

BASE_DIR = Path(__file__).resolve().parent

# Tempo sequenziale di riferimento (1 core) per N = 1.000.000
T_SEQ_1M = 337.75


def load_scaling_csv(csv_path: Path) -> pd.DataFrame:
    df = pd.read_csv(csv_path, skipinitialspace=True)
    df["Total_Time"] = df["CPU_Time"] + df["Net_Time"]
    return df


def analyze_strong_scaling(csv_path: Path, output_path: Path, title_suffix: str = "", t_seq: float = T_SEQ_1M) -> pd.DataFrame:
    df = load_scaling_csv(csv_path)

    # Speedup rispetto al tempo sequenziale (1 core): S_p = T_seq / T_p
    df["Speedup_CPU"] = t_seq / df["CPU_Time"]
    df["Speedup_Total"] = t_seq / df["Total_Time"]

    # Efficienza parallela assoluta (%): E_p = T_seq / (p * T_p) * 100
    df["Efficiency_CPU"] = (df["Speedup_CPU"] / df["Total_Cores"]) * 100.0
    df["Efficiency_Total"] = (df["Speedup_Total"] / df["Total_Cores"]) * 100.0

    suffix_str = f" ({title_suffix})" if title_suffix else ""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5.5), dpi=120)

    # 1. Grafico Speedup
    ax1.plot(
        df["Total_Cores"],
        df["Speedup_Total"],
        marker="s",
        linewidth=2.2,
        color="#ff7f0e",
        label="Speedup Totale (CPU+Net)",
    )
    ax1.plot(
        df["Total_Cores"],
        df["Speedup_CPU"],
        marker="o",
        linewidth=1.8,
        linestyle=":",
        color="#1f77b4",
        label="Speedup Solo CPU",
    )
    ax1.plot(
        df["Total_Cores"],
        df["Total_Cores"],
        linestyle="--",
        color="gray",
        alpha=0.8,
        label="Speedup Ideale ($S_p = p$)",
    )

    ax1.set_title(f"Strong Scaling: Speedup vs Core Totali{suffix_str}", fontsize=13, fontweight="bold")
    ax1.set_xlabel("Numero di Core Totali ($p$)", fontsize=11)
    ax1.set_ylabel("Speedup ($T_{seq} / T_p$)", fontsize=11)
    ax1.set_xticks(df["Total_Cores"])
    ax1.grid(True, linestyle=":", alpha=0.6)
    ax1.legend(frameon=True)

    # 2. Grafico Efficienza Parallela
    ax2.plot(
        df["Total_Cores"],
        df["Efficiency_Total"],
        marker="s",
        linewidth=2.2,
        color="#ff7f0e",
        label="Efficienza Totale (%)",
    )
    ax2.plot(
        df["Total_Cores"],
        df["Efficiency_CPU"],
        marker="o",
        linewidth=1.8,
        linestyle=":",
        color="#1f77b4",
        label="Efficienza Solo CPU (%)",
    )
    ax2.axhline(
        100.0,
        linestyle="--",
        color="gray",
        alpha=0.8,
        label="Efficienza Ideale (100%)",
    )

    ax2.set_title(f"Strong Scaling: Efficienza Parallela Assoluta{suffix_str}", fontsize=13, fontweight="bold")
    ax2.set_xlabel("Numero di Core Totali ($p$)", fontsize=11)
    ax2.set_ylabel("Efficienza (%) [ $T_{seq} / (p \\cdot T_p)$ ]", fontsize=11)
    ax2.set_ylim(0, max(110.0, df["Efficiency_CPU"].max() + 10.0))
    ax2.set_xticks(df["Total_Cores"])
    ax2.grid(True, linestyle=":", alpha=0.6)
    ax2.legend(frameon=True)

    plt.tight_layout()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(output_path, dpi=300)
    plt.close()

    return df


def analyze_strong_scaling_comparison(aos_csv: Path, soa_csv: Path, output_path: Path, t_seq: float = T_SEQ_1M) -> None:
    df_aos = load_scaling_csv(aos_csv)
    df_soa = load_scaling_csv(soa_csv)

    for df in (df_aos, df_soa):
        df["Speedup_Total"] = t_seq / df["Total_Time"]
        df["Efficiency_Total"] = (df["Speedup_Total"] / df["Total_Cores"]) * 100.0

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5.5), dpi=120)

    # 1. Confronto Speedup AoS vs SoA
    ax1.plot(
        df_aos["Total_Cores"],
        df_aos["Speedup_Total"],
        marker="s",
        linewidth=2.2,
        color="#ff7f0e",
        label="Speedup AoS (CPU+Net)",
    )
    ax1.plot(
        df_soa["Total_Cores"],
        df_soa["Speedup_Total"],
        marker="^",
        linewidth=2.2,
        color="#2ca02c",
        label="Speedup SoA (CPU+Net)",
    )
    ax1.plot(
        df_aos["Total_Cores"],
        df_aos["Total_Cores"],
        linestyle="--",
        color="gray",
        alpha=0.8,
        label="Speedup Ideale ($S_p = p$)",
    )

    ax1.set_title("Strong Scaling: Confronto Speedup (AoS vs SoA)", fontsize=13, fontweight="bold")
    ax1.set_xlabel("Numero di Core Totali ($p$)", fontsize=11)
    ax1.set_ylabel("Speedup ($T_{seq} / T_p$)", fontsize=11)
    ax1.set_xticks(df_aos["Total_Cores"])
    ax1.grid(True, linestyle=":", alpha=0.6)
    ax1.legend(frameon=True)

    # 2. Confronto Efficienza AoS vs SoA
    ax2.plot(
        df_aos["Total_Cores"],
        df_aos["Efficiency_Total"],
        marker="s",
        linewidth=2.2,
        color="#ff7f0e",
        label="Efficienza AoS (%)",
    )
    ax2.plot(
        df_soa["Total_Cores"],
        df_soa["Efficiency_Total"],
        marker="^",
        linewidth=2.2,
        color="#2ca02c",
        label="Efficienza SoA (%)",
    )
    ax2.axhline(
        100.0,
        linestyle="--",
        color="gray",
        alpha=0.8,
        label="Efficienza Ideale (100%)",
    )

    max_eff = max(df_aos["Efficiency_Total"].max(), df_soa["Efficiency_Total"].max())
    ax2.set_title("Strong Scaling: Confronto Efficienza (AoS vs SoA)", fontsize=13, fontweight="bold")
    ax2.set_xlabel("Numero di Core Totali ($p$)", fontsize=11)
    ax2.set_ylabel("Efficienza (%) [ $T_{seq} / (p \\cdot T_p)$ ]", fontsize=11)
    ax2.set_ylim(0, max(110.0, max_eff + 10.0))
    ax2.set_xticks(df_aos["Total_Cores"])
    ax2.grid(True, linestyle=":", alpha=0.6)
    ax2.legend(frameon=True)

    plt.tight_layout()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(output_path, dpi=300)
    plt.close()


def analyze_weak_scaling(csv_path: Path, output_path: Path) -> pd.DataFrame:
    df = load_scaling_csv(csv_path)

    # Baseline alla configurazione minima (p_0 = 4 core, N_0 = 100.000 corpi)
    base_row = df.iloc[0]
    p0 = float(base_row["Total_Cores"])
    n0 = float(base_row["NumBodies"])
    t0_total = float(base_row["Total_Time"])
    t0_cpu = float(base_row["CPU_Time"])

    # Fattore teorico O(N log2 N) per core al crescere di N: log2(N) / log2(N_0)
    log_factor = np.log2(df["NumBodies"]) / np.log2(n0)

    # Efficienza Weak Scaling rispetto alla baseline a p_0 core: E_weak(p) = (T_0 / T_p) * 100
    df["Efficiency_Weak_Total"] = (t0_total / df["Total_Time"]) * 100.0
    df["Efficiency_Weak_CPU"] = (t0_cpu / df["CPU_Time"]) * 100.0
    df["Efficiency_Weak_NlogN"] = df["Efficiency_Weak_Total"] * log_factor

    # Speedup Scalato (Gustafson) riferito a p core: S_scaled(p) = p * (T_0 / T_p)
    df["Scaled_Speedup_Total"] = df["Total_Cores"] * (t0_total / df["Total_Time"])
    df["Scaled_Speedup_NlogN"] = df["Scaled_Speedup_Total"] * log_factor

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5.5), dpi=120)

    # 1. Grafico Scaled Speedup (Weak Scaling)
    ax1.plot(
        df["Total_Cores"],
        df["Scaled_Speedup_Total"],
        marker="s",
        linewidth=2.2,
        color="#ff7f0e",
        label="Speedup Scalato ($p \\cdot T_{p_0} / T_p$)",
    )
    ax1.plot(
        df["Total_Cores"],
        df["Scaled_Speedup_NlogN"],
        marker="o",
        linewidth=1.8,
        linestyle=":",
        color="#2ca02c",
        label="Speedup Normalizzato $O(N \\log N)$",
    )
    ax1.plot(
        df["Total_Cores"],
        df["Total_Cores"],
        linestyle="--",
        color="gray",
        alpha=0.8,
        label="Scaling Ideale ($S_p = p$)",
    )

    ax1.set_title("Weak Scaling: Speedup Scalato (25k corpi/core)", fontsize=13, fontweight="bold")
    ax1.set_xlabel("Numero di Core Totali ($p$, con $N = 25\\text{k} \\cdot p$)", fontsize=11)
    ax1.set_ylabel("Speedup Scalato", fontsize=11)
    ax1.set_xticks(df["Total_Cores"])
    ax1.grid(True, linestyle=":", alpha=0.6)
    ax1.legend(frameon=True)

    # 2. Grafico Efficienza Weak Scaling
    ax2.plot(
        df["Total_Cores"],
        df["Efficiency_Weak_Total"],
        marker="s",
        linewidth=2.2,
        color="#ff7f0e",
        label="Efficienza Weak Totale ($T_{p_0} / T_p$)",
    )
    ax2.plot(
        df["Total_Cores"],
        df["Efficiency_Weak_NlogN"],
        marker="o",
        linewidth=1.8,
        linestyle=":",
        color="#2ca02c",
        label="Efficienza Corretta $O(N \\log N)$",
    )
    ax2.axhline(
        100.0,
        linestyle="--",
        color="gray",
        alpha=0.8,
        label=f"Efficienza Ideale (100% a $p_0={int(p0)}$)",
    )

    ax2.set_title("Weak Scaling: Efficienza Parallela (%)", fontsize=13, fontweight="bold")
    ax2.set_xlabel("Numero di Core Totali ($p$, con $N = 25\\text{k} \\cdot p$)", fontsize=11)
    ax2.set_ylabel("Efficienza Weak (%) [ $T_{p_0} / T_p \\cdot 100$ ]", fontsize=11)
    ax2.set_ylim(0, 115.0)
    ax2.set_xticks(df["Total_Cores"])
    ax2.grid(True, linestyle=":", alpha=0.6)
    ax2.legend(frameon=True)

    plt.tight_layout()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(output_path, dpi=300)
    plt.close()

    return df


def analyze_hybrid_vs_mpi_only(hybrid_csv: Path, mpi_only_csv: Path, output_path: Path, mode_label: str = "SoA", t_seq: float = T_SEQ_1M) -> None:
    df_hyb = load_scaling_csv(hybrid_csv)
    df_mpi = load_scaling_csv(mpi_only_csv)

    for df in (df_hyb, df_mpi):
        df["Speedup_Total"] = t_seq / df["Total_Time"]
        df["Efficiency_Total"] = (df["Speedup_Total"] / df["Total_Cores"]) * 100.0

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5.5), dpi=120)

    # 1. Confronto Speedup: Ibrido (4 MPI x t OMP) vs Solo MPI (p MPI x 1 OMP)
    ax1.plot(
        df_hyb["Total_Cores"],
        df_hyb["Speedup_Total"],
        marker="^",
        linewidth=2.2,
        color="#2ca02c",
        label=f"Ibrido 4 MPI × t OMP ({mode_label})",
    )
    ax1.plot(
        df_mpi["Total_Cores"],
        df_mpi["Speedup_Total"],
        marker="o",
        linewidth=2.2,
        color="#d62728",
        label=f"Solo MPI p × 1 OMP ({mode_label})",
    )
    ax1.plot(
        df_hyb["Total_Cores"],
        df_hyb["Total_Cores"],
        linestyle="--",
        color="gray",
        alpha=0.8,
        label="Speedup Ideale ($S_p = p$)",
    )

    ax1.set_title(f"Strong Scaling ({mode_label}): Ibrido MPI+OMP vs Solo MPI", fontsize=13, fontweight="bold")
    ax1.set_xlabel("Numero di Core Totali ($p$)", fontsize=11)
    ax1.set_ylabel("Speedup ($T_{seq} / T_p$)", fontsize=11)
    ax1.set_xticks(df_hyb["Total_Cores"])
    ax1.grid(True, linestyle=":", alpha=0.6)
    ax1.legend(frameon=True)

    # 2. Confronto Efficienza
    ax2.plot(
        df_hyb["Total_Cores"],
        df_hyb["Efficiency_Total"],
        marker="^",
        linewidth=2.2,
        color="#2ca02c",
        label=f"Efficienza Ibrido ({mode_label})",
    )
    ax2.plot(
        df_mpi["Total_Cores"],
        df_mpi["Efficiency_Total"],
        marker="o",
        linewidth=2.2,
        color="#d62728",
        label=f"Efficienza Solo MPI ({mode_label})",
    )
    ax2.axhline(
        100.0,
        linestyle="--",
        color="gray",
        alpha=0.8,
        label="Efficienza Ideale (100%)",
    )

    max_eff = max(df_hyb["Efficiency_Total"].max(), df_mpi["Efficiency_Total"].max())
    ax2.set_title(f"Efficienza ({mode_label}): Ibrido MPI+OMP vs Solo MPI", fontsize=13, fontweight="bold")
    ax2.set_xlabel("Numero di Core Totali ($p$)", fontsize=11)
    ax2.set_ylabel("Efficienza (%) [ $T_{seq} / (p \\cdot T_p)$ ]", fontsize=11)
    ax2.set_ylim(0, max(110.0, max_eff + 10.0))
    ax2.set_xticks(df_hyb["Total_Cores"])
    ax2.grid(True, linestyle=":", alpha=0.6)
    ax2.legend(frameon=True)

    plt.tight_layout()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(output_path, dpi=300)
    plt.close()


def main() -> None:
    strong_dir = BASE_DIR / "scaling" / "strong"
    weak_dir = BASE_DIR / "scaling" / "weak"

    aos_strong_csv = strong_dir / "results_scaling.data"
    soa_strong_csv = strong_dir / "results_scaling_soa.data"
    aos_mpi_only_csv = strong_dir / "results_parallel_mpi_only.data"
    soa_mpi_only_csv = strong_dir / "results_parallel_soa_mpi_only.data"
    weak_csv = weak_dir / "results_scaling.data"

    if aos_strong_csv.exists():
        analyze_strong_scaling(
            aos_strong_csv,
            strong_dir / "benchmark_efficiency.png",
            title_suffix="AoS Ibrido",
        )
        print(f"Generato: {strong_dir / 'benchmark_efficiency.png'}")

    if soa_strong_csv.exists():
        analyze_strong_scaling(
            soa_strong_csv,
            strong_dir / "benchmark_efficiency_soa.png",
            title_suffix="SoA Ibrido",
        )
        print(f"Generato: {strong_dir / 'benchmark_efficiency_soa.png'}")

    if aos_strong_csv.exists() and soa_strong_csv.exists():
        analyze_strong_scaling_comparison(
            aos_strong_csv,
            soa_strong_csv,
            strong_dir / "benchmark_efficiency_comparison.png",
        )
        print(f"Generato: {strong_dir / 'benchmark_efficiency_comparison.png'}")

    if aos_mpi_only_csv.exists():
        analyze_strong_scaling(
            aos_mpi_only_csv,
            strong_dir / "benchmark_efficiency_mpi_only.png",
            title_suffix="AoS Solo MPI",
        )
        print(f"Generato: {strong_dir / 'benchmark_efficiency_mpi_only.png'}")
        if aos_strong_csv.exists():
            analyze_hybrid_vs_mpi_only(
                aos_strong_csv,
                aos_mpi_only_csv,
                strong_dir / "benchmark_hybrid_vs_mpi_only_aos.png",
                mode_label="AoS",
            )
            print(f"Generato: {strong_dir / 'benchmark_hybrid_vs_mpi_only_aos.png'}")

    if soa_mpi_only_csv.exists():
        analyze_strong_scaling(
            soa_mpi_only_csv,
            strong_dir / "benchmark_efficiency_soa_mpi_only.png",
            title_suffix="SoA Solo MPI",
        )
        print(f"Generato: {strong_dir / 'benchmark_efficiency_soa_mpi_only.png'}")
        if soa_strong_csv.exists():
            analyze_hybrid_vs_mpi_only(
                soa_strong_csv,
                soa_mpi_only_csv,
                strong_dir / "benchmark_hybrid_vs_mpi_only_soa.png",
                mode_label="SoA",
            )
            print(f"Generato: {strong_dir / 'benchmark_hybrid_vs_mpi_only_soa.png'}")

    if weak_csv.exists():
        analyze_weak_scaling(
            weak_csv,
            weak_dir / "benchmark_weak_scaling.png",
        )
        print(f"Generato: {weak_dir / 'benchmark_weak_scaling.png'}")


if __name__ == "__main__":
    main()