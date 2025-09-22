import random

# Generar 1000 números decimales entre 0 y 1
datos = [round(random.uniform(0, 1), 3) for _ in range(1000)]

# Guardar en un archivo txt
with open("datos_random.txt", "w") as archivo:
    for numero in datos:
        archivo.write(f"{numero}\n")
