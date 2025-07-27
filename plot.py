import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

groups = {
    "":  "1024 byte key / 1032 byte value",
    "8": "8 byte key / 8 byte value",
    "r": "random 1024 byte key / 1032 byte value",
    "r8": "random 8 byte key / 8 byte value",
}

implementations = ["boost", "ska", "swiss"]
operations      = ["Insert", "Lookup", "Delete"]
data_dir        = ".temp"

for suffix, desc in groups.items():
    for op in operations:
        plt.figure(figsize=(6,4))

        for impl in implementations:
            fn = os.path.join(data_dir, f"{impl}{suffix}.csv")
            if not os.path.exists(fn):
                continue

            df    = pd.read_csv(fn)
            df_op = df[df['operation'] == op]

            x = df_op['count'].to_numpy()
            y = df_op['avg_ns'].to_numpy()

            if y.size == 0:
                continue

            mu, sigma = y.mean(), y.std()
            mask = y <= mu + 4 * sigma
            x, y = x[mask], y[mask]

            if x.size > 1:
                xi = np.linspace(x.min(), x.max(), 500)
                yi = np.interp(xi, x, y)
                plt.plot(xi, yi, label=impl)
            else:
                plt.plot(x, y, 'o', label=impl)

        plt.xlabel(r'Count ($n$)')
        plt.ylabel(r'Average latency (ns)')
        plt.title(f"{op} Operation ({desc})")
        plt.legend(title="Impl")
        plt.tight_layout()
        out = f"{op.lower()}_{suffix or 'nosuffix'}.png"
        plt.savefig(out, dpi=300)
        plt.close()

for suffix, desc in groups.items():
    grp_name = suffix or "nosuffix"
    print(f"## Group `{grp_name}`: {desc}\n")

    for op in operations:
        rows = []
        for impl in implementations:
            fn = os.path.join(data_dir, f"{impl}{suffix}.csv")
            if not os.path.exists(fn):
                continue
            df_all = pd.read_csv(fn)
            y_all  = df_all[df_all['operation'] == op]['avg_ns'].to_numpy()
            if y_all.size == 0:
                continue
            μ    = y_all.mean()
            σ    = y_all.std()
            y     = y_all[y_all <= μ + 4*σ]
            rows.append({
                "Impl":      impl,
                "Mean (ns)": f"{y.mean():.2f}",
                "Std (ns)":  f"{y.std():.2f}"
            })

        if not rows:
            continue

        print(f"### Operation: {op}\n")
        headers = ["Impl", "Mean (ns)", "Std (ns)"]
        print("| " + " | ".join(headers) + " |")
        print("| " + " | ".join("---" for _ in headers) + " |")
        for row in rows:
            print("| " + " | ".join(row[h] for h in headers) + " |")
        print()
