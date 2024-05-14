import numpy as np

# Parameters for Pareto distribution
alphas = [1.5, 2, 2.5, 3]  # Different values of alpha
size = 9  # Number of items per group
max_value = 800  # Maximum value for scaling
groups_per_alpha = 10  # Number of groups per alpha value

# Create an empty list to store all the mapped data
round_data = []

# Generate data for each alpha value
for alpha in alphas:
    # Generate ten groups of data for each alpha
    for _ in range(groups_per_alpha):
        valid_data_found = False
        while not valid_data_found:
            # Generate Pareto distribution data and scale
            pareto_random_numbers = np.random.pareto(alpha, size)
            
            # Normalize this group of random numbers, then scale
            data = (pareto_random_numbers / pareto_random_numbers.max()) * max_value
            
            # Round the data
            round_group = [round(y) for y in data]
            
             # Check if any value is 0, if not, add to round_data
            if min(round_group) >= 10:
                round_data.append(round_group)
                valid_data_found = True

# Write to file
with open('IOBandwidth.txt', 'w') as f:
    for group in round_data:
        f.write(' '.join(map(str, group)) + '\n')
