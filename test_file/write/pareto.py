import numpy as np
import matplotlib.pyplot as plt

#生成多节点高带宽
def generate_scaled_pareto(N, alpha, scale, range_max):
    scaled_traffic = np.full(N, range_max + 1, dtype=int)
    
    while np.max(scaled_traffic) > range_max:
        # 生成 N 个节点的帕累托分布
        pareto_values = np.random.pareto(alpha, N)
        #print("pareto_values:", pareto_values)

        # 将分布缩放到指定的范围
        scaled_traffic = pareto_values * scale
    
    # 返回整数值
    return np.round(scaled_traffic).astype(int)

# 设置参数
N = 14  # 节点数
alpha = 2 # 帕累托分布的形状参数：越小下降越慢-流量越大-带宽越小，越大下降越快-流量越小-带宽越大，尽可能覆盖范围
scale = 900  # 缩放参数
traffic_max = 900  # 流量范围的最大值
bandwidth_max = 1000  # 总带宽为 500 Mbps

# 生成节点的网络流量
traffic = generate_scaled_pareto(N, alpha, scale, traffic_max)

# 计算每个节点的实际网络带宽
actual_bandwidth= bandwidth_max - traffic

# 打印结果
print("traffic(Mbps):", traffic)
print("actual_bandwidth(Mbps):", actual_bandwidth)
print("asum(actual_bandwidth):", np.sum(actual_bandwidth))

# 绘制直方图
# plt.figure(figsize=(12, 6))
# plt.hist(traffic, bins=20, density=True, alpha=0.7, color='blue', label='Network Traffic')
# plt.hist(actual_bandwidth, bins=20, density=True, alpha=0.7, color='green', label='Actual Bandwidth per Node')
# plt.title('Network Traffic and Actual Bandwidth per Node')
# plt.xlabel('Value (Mbps)')
# plt.ylabel('Probability Density')
# plt.legend()
# plt.show()

#生成多节点小带宽
def generate_scaled_pareto2(N, alpha, scale, range_min, range_max):
    scaled_traffic = np.full(N, range_max + 1, dtype=int)
    
    while np.max(scaled_traffic) > range_max:
        # 生成 N 个节点的帕累托分布
        pareto_values = np.random.pareto(alpha, N)
        #print("pareto_values:", pareto_values)

        # 将分布缩放到指定的范围
        scaled_traffic = pareto_values * scale + range_min
    
    # 返回整数值
    return np.round(scaled_traffic).astype(int)
# 设置参数
# N = 13  # 节点数
alpha = 1.2   # 帕累托分布的形状参数：越小下降越慢-带宽越大，越大下降越快-带宽越小，尽可能覆盖范围
scale = 900  # 缩放参数
bandwidth_max = 1000  # 总带宽为 500 Mbps
bandwidth_min = 100

# 生成节点的网络流量
actual_bandwidth2 = generate_scaled_pareto2(N, alpha, scale, bandwidth_min, bandwidth_max)

print("actual_bandwidth(Mbps):", actual_bandwidth2)
print("asum(actual_bandwidth):", np.sum(actual_bandwidth2))

# 绘制直方图
# plt.figure(figsize=(12, 6))
# # plt.hist(traffic, bins=20, density=True, alpha=0.7, color='blue', label='Network Traffic')
# plt.hist(actual_bandwidth2, bins=20, density=True, alpha=0.7, color='green', label='Actual Bandwidth per Node')
# plt.title('Network Traffic and Actual Bandwidth per Node')
# plt.xlabel('Value (Mbps)')
# plt.ylabel('Probability Density')
# plt.legend()
# plt.show()


#官方源码
# import numpy as np
# import matplotlib.pyplot as plt
# a, m = 2., 2.  # shape and mode
# s = (np.random.default_rng().pareto(a, 10) + 1) * 500
# count, bins, _ = plt.hist(s, 100, density=True)
# fit = a*m**a / bins**(a+1)
# plt.plot(bins, max(count)*fit/max(fit), linewidth=2, color='r')
# plt.show()