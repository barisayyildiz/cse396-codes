# Read the .obj file
with open('3d.obj', 'r') as f:
    lines = f.readlines()
f.close()

# Initialize min and max values
min_values = [float('inf'), float('inf'), float('inf')]
max_values = [-float('inf'), -float('inf'), -float('inf')]

# Iterate through the lines to find min and max values
for line in lines:
    if line.startswith('v '):
        x, y, z = map(float, line.split()[1:])
        min_values = [min(min_values[i], x, y, z) for i in range(3)]
        max_values = [max(max_values[i], x, y, z) for i in range(3)]

# Calculate the range for normalization
ranges = [max_values[i] - min_values[i] for i in range(3)]
# Normalize the vertices and write to a new file
with open('output.obj', 'w') as f:
    for line in lines:
        if line.startswith('v '):
            x, y, z = map(float, line.split()[1:])
            normalized_x = (x - min_values[0]) / ranges[0]
            normalized_y = (y - min_values[1]) / ranges[1]
            normalized_z = (z - min_values[2]) / ranges[2]
            f.write(f'v {normalized_x} {normalized_y} {normalized_z}\n')
            f.write(f'vt {normalized_x} {normalized_y}\n')
        else:
            x, y, z = map(int, line.split()[1:])
            f.write(f'f {x}/{x} {y}/{y} {z}/{z}\n')
f.close()

with open('output.obj', 'r') as f:
    lines = f.readlines()
f.close()


