### Обзор проекта
**Цель**: реализовать и сравнить два подхода к сжатию дорожных графов — *manual* (baseline) и *algo* (motif‑based coarsening), собрать статистику по набору городов и визуализировать результаты.  
**Ядро**: C++ бинарник `./compare` (выполняет coarsening и пишет артефакты).  
**Вспомогательные скрипты**: Python для подготовки данных, запуска, визуализации и агрегации метрик.

---


### Быстрый старт
1. **Клонировать репозиторий** и перейти в корень проекта.
2. **Создать виртуальное окружение и установить зависимости**:
```bash
python3 -m venv venv
source venv/bin/activate
python3 -m pip install --upgrade pip
python3 -m pip install -r experiments/requirements.txt
```
3. **Скачать или подготовить edgelist для городов** (в `data_cities/`) либо сгенерировать их скриптом:
```bash
# тестовый прогон (малые графы)
python3 download_and_sample_from_txt.py --cities_file experiments/cities.txt --target_nodes 200 --outdir data_cities --sleep 1.0

# полный прогон (1000 узлов)
python3 download_and_sample_from_txt.py --cities_file experiments/cities.txt --target_nodes 1000 --outdir data_cities --sleep 1.0
```
4. **Запустить сравнение и собрать результаты**:
```bash
chmod +x run_compare_and_plot_all.sh
./run_compare_and_plot_all.sh
```
5. **Построить агрегированные графики и таблицы**:
```bash
python3 plot_compression_and_clusters.py --results_root experiments_results --out_dir results/plots
python3 analyze_and_plot.py --results_root experiments_results --out_dir results/plots
```

---

### Структура репозитория
| Путь | Описание |
|---|---|
| **`compare`** | C++ бинарник — основной исполнитель coarsening. |
| **`data_cities/`** | Edgelist файлы по городам (один файл на город). |
| **`experiments/`** | Скрипты для запуска, подготовки и визуализации; `cities.txt`. |
| **`experiments_results/`** | Папка с результатами для каждого города: `results/compare_results.csv`, `algo_old2new.txt`, `manual_old2new.txt`, `orig_remapped.edgelist`, `layout.json`. |
| **`results/plots/`** | Итоговые PNG графики: compression, cluster counts, three‑panel визуализации. |
| **`src/`** | Исходники C++ (`main.cpp`, `algo.cpp`, `manual.cpp`). |

---

### Основные скрипты и их назначение
| Скрипт | Что делает | Результат |
|---|---:|---|
| `download_and_sample_from_txt.py` | Скачивает OSM графы по списку городов и сохраняет remapped edgelist | `data_cities/<City>.edgelist` |
| `run_compare_and_plot_all.sh` | Последовательно запускает `./compare` для каждого edgelist и вызывает визуализацию | `experiments_results/<City>/results/*` и `results/plots/*` |
| `plot_graphs_consistent_layout.py` | Рисует Original / Algo / Manual с единым layout | `results/<city>_algo_colored.png`, `results/<city>_manual_colored.png` |
| `plot_compression_and_clusters.py` | Строит графики сравнения сжатия и распределения размеров кластеров | `results/plots/compression_*.png`, `results/plots/algo_cluster_counts_by_city.png` |
| `analyze_and_plot.py` | Аггрегирует `compare_results.csv` и `algo_old2new.txt`, сохраняет CSV‑сводки | `results/summary/*.csv` |

---

### Что искать в выходных данных
- **`experiments_results/<City>/results/compare_results.csv`** — основная таблица с колонками `algorithm, orig_nodes, coarsened_nodes, compression_ratio, time_ms`.  
- **`experiments_results/<City>/results/algo_old2new.txt`** — mapping `old new` для анализа размеров кластеров.  
- **Графики**:
  - `results/plots/compression_mean_by_city.png` — среднее сжатие по городам и алгоритмам.  
  - `results/plots/compression_box_by_city.png` — распределение compression_ratio.  
  - `results/plots/algo_cluster_counts_by_city.png` — сколько кластеров каждого размера получилось у `algo`.  
  - `results/plots/algo_cluster_size_hist_all_cities.png` — гистограмма размеров кластеров по всем городам.

---


### Примеры команд для типичных задач
**Запустить весь pipeline последовательно**
```bash
# подготовка окружения
source venv/bin/activate

# подготовка данных (если нужно)
python3 download_and_sample_from_txt.py --cities_file experiments/cities.txt --target_nodes 1000 --outdir data_cities

# запустить compare и визуализацию по всем городам
./run_compare_and_plot_all.sh

# построить агрегированные графики и таблицы
python3 plot_compression_and_clusters.py --results_root experiments_results --out_dir results/plots
python3 analyze_and_plot.py --results_root experiments_results --out_dir results/plots
```

**Только визуализация по уже существующим результатам**
```bash
python3 plot_graphs_consistent_layout.py --results_dir experiments_results --data_dir data_cities --out_dir results/plots
```

---

### Полезные заметки
- **Формат edgelist**: строки `u v` с целыми индексами; рекомендуется remap 0..n-1 перед запуском `compare`.  
- **Параметры EF и max_motifs** задаются в исходниках C++; для воспроизводимости фиксируйте версии бинарника и параметры запуска.  
- **CSV‑сводки** находятся в `results/summary/` — удобно вставлять в LaTeX‑отчёт (см. `Отчет_1курс_updated.tex`).

---

## Результаты статистического эксперимента

**Кратко о эксперименте.** Мы провели серию запусков на наборе из 10 дорожных графов (по одному edgelist на город). Для каждого города были выполнены оба алгоритма: `algo` (motif‑based) и `manual` (baseline). По каждому запуску собраны метрики и приведена визуализация.
<img width="2400" height="1200" alt="compression_by_city" src="https://github.com/user-attachments/assets/298f3978-df0c-4e92-8328-bf5e4b56148c" />

## Также представлены графики по городам для алгоритмов сжатия в сравнении 

<img width="3000" height="1000" alt="Baiyin_China_three_panel" src="https://github.com/user-attachments/assets/c8b94bed-0d5c-4c39-8a15-7e3a0e2404b3" />
<img width="3000" height="1000" alt="Barcelona_Spain_three_panel" src="https://github.com/user-attachments/assets/93e95871-edd3-4435-ae4d-d958e9f6324b" />
<img width="3000" height="1000" alt="Buenos_Aires_Argentina_three_panel" src="https://github.com/user-attachments/assets/7769f508-a3a5-486a-a5d2-c1fc3d82da23" />
<img width="3000" height="1000" alt="Chicago_Illinois_USA_three_panel" src="https://github.com/user-attachments/assets/0cfd85f9-addc-4ed3-abfa-d3b995c577eb" />
<img width="3000" height="1000" alt="Helsinki_Finland_three_panel" src="https://github.com/user-attachments/assets/b3bbc2ca-b819-494f-9d99-934be43f7f42" />
<img width="3000" height="1000" alt="Manhattan_New_York_USA_three_panel" src="https://github.com/user-attachments/assets/2343d7ab-9c33-4978-b574-8e99376c0172" />
<img width="3000" height="1000" alt="Melbourne_Australia_three_panel" src="https://github.com/user-attachments/assets/4349eeca-85ea-4662-947f-b34c61702a5c" />
<img width="3000" height="1000" alt="Moscow_Russia_three_panel" src="https://github.com/user-attachments/assets/0a3d2bab-b93c-49a0-8068-4c6142770b72" />
<img width="3000" height="1000" alt="Portland_Oregon_USA_three_panel" src="https://github.com/user-attachments/assets/857631f7-029d-44dc-8ae4-c2bfe076353e" />
<img width="3000" height="1000" alt="San_Francisco_California_USA_three_panel" src="https://github.com/user-attachments/assets/e149257a-aaad-4a53-b4ee-d2fa33d46a63" />
