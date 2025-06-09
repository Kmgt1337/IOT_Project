from influxdb_client import InfluxDBClient
from influxdb_client.client.write_api import SYNCHRONOUS
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from matplotlib.animation import FuncAnimation

# Ustawienia połączenia z InfluxDB
url = "http://localhost:8086"
token = "fVKUeejSWPsJ_ODhjSPg2V-EpCjde-7WKwP79MRINVY8Oj1RZPyf52qKvcsD9RG_dWwx-PUv5VZcoATHrchekQ=="
org = "6dbeb66792bbf8e2"
bucket = "POMIARY"

# Połączenie z InfluxDB
client = InfluxDBClient(url=url, token=token, org=org)
query_api = client.query_api()

# Zapytanie do InfluxDB
query = f'''
from(bucket: "{bucket}")
  |> range(start: -1h)
  |> filter(fn: (r) => contains(value: r._measurement, set: ["cisnienie", "temperatura", "wysokosc"]))
  |> pivot(rowKey:["_time"], columnKey: ["_measurement"], valueColumn: "_value")
  |> sort(columns:["_time"], desc: false)
'''

# Tworzenie wykresów
fig, axs = plt.subplots(3, 1, figsize=(14, 8))
fig.tight_layout(pad=4)

# Format czasu z dokładnością do sekundy
time_format = mdates.DateFormatter('%H:%M:%S')
minute_locator = mdates.MinuteLocator(interval=1)  # Znaczniki co 1 minutę

def update(frame):
    tables = query_api.query_data_frame(org=org, query=query)
    if tables.empty:
        print("Brak danych w podanym zakresie.")
        return

    df = tables
    df['_time'] = pd.to_datetime(df['_time'])
    df.set_index('_time', inplace=True)

    for ax, measurement, color, marker, ylabel in zip(
            axs,
            ['temperatura', 'cisnienie', 'wysokosc'],
            ['red', 'blue', 'green'],
            ['o', 's', '^'],
            ['Temperatura [°C]', 'Ciśnienie [hPa]', 'Wysokość [m]']):
        
        ax.clear()
        ax.plot(df.index, df[measurement], color=color, marker=marker, linestyle='-')
        ax.set_title(f'{ylabel.split()[0]} w czasie')
        ax.set_ylabel(ylabel)
        ax.grid(True)
        ax.xaxis.set_major_formatter(time_format)
        ax.xaxis.set_major_locator(minute_locator)
        ax.tick_params(axis='x', rotation=45)

    axs[2].set_xlabel('Czas')

    plt.tight_layout()

# Animacja – odświeżanie co 5 sekund
ani = FuncAnimation(fig, update, interval=1000)

plt.show()

# Zamknięcie klienta po zamknięciu wykresu
client.close()
