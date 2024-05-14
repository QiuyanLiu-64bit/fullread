from scipy.optimize import minimize
import numpy as np
import sys

if len(sys.argv) < 6:
        print("Usage: python3 read_opt.py output_file_path TOTAL_SIZE EC_N EC_K R1 R2 ... RN")
        sys.exit(1)

output_file_path = sys.argv[1]
TOTAL_SIZE_ORI = int(sys.argv[2])
EC_N = int(sys.argv[3])
EC_K = int(sys.argv[4])
R_ORI = np.array([float(x) for x in sys.argv[5:5+EC_N]])

def adjust_and_round_array(arr, TOTAL_SIZE, EC_K):
    arr_rounded = np.round(arr)

    adjustment = TOTAL_SIZE / EC_K - arr_rounded.sum()
    arr_rounded[-1] += adjustment

    while np.any(arr_rounded <= 0):
        max_index = np.argmax(arr_rounded)
        arr_rounded[max_index] -= 1
        zero_index = np.argmin(arr_rounded)
        arr_rounded[zero_index] += 1

    return arr_rounded

TOTAL_SIZE = 10
R = R_ORI / R_ORI.sum() 

BLOCK_SIZE = TOTAL_SIZE / EC_K

A = np.ones((EC_N, EC_N))
for i in range(EC_N):
    for j in range(EC_N - EC_K):
        col = (i + j) % EC_N
        A[i, col] = 0


constraints = ({'type': 'eq', 'fun': lambda p:  np.sum(p) - (BLOCK_SIZE)})

def objective(p):
    r = np.dot(A, p)
    r = r / r.sum()
    return (r / R).max()

bounds = [(0, BLOCK_SIZE ) for _ in range(EC_N)]

p_initial = np.full(EC_N, TOTAL_SIZE / (EC_N * EC_K))

result_p = minimize(objective, p_initial, method='SLSQP', constraints=constraints, bounds=bounds)

result_p.x *= (TOTAL_SIZE_ORI / 10)

with open(output_file_path, 'w') as file: 
    if result_p.success:
        print("Optimization successful.")
        p_result_adjusted = adjust_and_round_array(result_p.x, TOTAL_SIZE_ORI, EC_K)
        print("Adjusted p_result: ", p_result_adjusted)
        file.write('\n'.join(map(str, p_result_adjusted)))
        
    else:
        print("Optimization failed.")
        default_p = np.full(EC_N, TOTAL_SIZE_ORI / (EC_K * EC_N))
        print("Using default r:", default_p)

        default_p_adjusted = adjust_and_round_array(default_p, TOTAL_SIZE_ORI, EC_K)
        print("Adjusted default r:", default_p_adjusted)
        file.write('\n'.join(map(str, default_p_adjusted)))
        
