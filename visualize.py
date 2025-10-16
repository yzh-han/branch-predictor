import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

def main():
    ##############################################################
    csv_path = 'results/trace_comparison.csv'
    df_row = pd.read_csv(csv_path)

    # draw trace comparison
    cols = [
        "DirectBranches_pct",
        "ConditionalBranches_pct",
        "TakenBranches_pct",
        "RegularBranches_pct",
        "FunctionCalls_pct",
        "FunctionReturns_pct",
        "HighlyPredictableAll_pct",
        "Top5HotspotPercentage"
    ]
    df = df_row[cols]
        
    draw_bar(
        df, xlabels=df_row["TraceName"],
        title='Trace Comparison',
        xlabel='', 
        ylabel='percentage (%)',
        save_path='results/plots_trace_comparison.png')
    
    print('Trace Comparison has been saved to results/plots_trace_comparison.png')

    #####################################################################
    csv_path = 'results/results_predict.csv'
    df_row = pd.read_csv(csv_path)
    df=df_row
    pivot_df = df.pivot_table(
        index='Predictor',
        columns='TraceFile',
        values='MispredictionRate',
        aggfunc='mean'
    )

    # rename columns
    pivot_df = pivot_df[['wrf_cutted', 'exchange2_cutted', 'gcc_cutted', 'leela_cutted']]
    pivot_df.columns = ['wrf', 'exchange2', 'gcc', 'leela']

    # calculate average
    pivot_df['Average'] = pivot_df.mean(axis=1)

    df_T = pivot_df.T.reset_index()

    draw_bar(
        df_data=df_T.iloc[:, 1:],
        xlabels=df_T.iloc[:, 0],
        title='Predictor Comparison',
        xlabel='',
        ylabel='Misprediction Rate (%)',
        save_path='results/plots_predictor_comparison.png'
    )

    print('Predictor Comparison has been saved to results/plots_predictor_comparison.png')

    ###########################################################
    # create 2-bit predictor comparison
    df_2b = pivot_df[0:4]
    df_2b = df_2b.iloc[[3,0,1,2], :]
    draw_bar(
        df_data=df_2b.iloc[:, :4],
        xlabels=df_2b.index,
        title='2-bit Predictor different table size Comparison',
        xlabel='',
        ylabel='Misprediction Rate (%)',
        save_path='results/plots_predictor_comparison_2b.png'
    )

    print('2-bit Predictor Comparison has been saved to results/plots_predictor_comparison_2b.png')
    

def draw_bar(
		df_data:pd.DataFrame,
		xlabels,
		title='',
		xlabel='',
		ylabel='',
		save_path='plots.png',
	):
	width = 0.12

	xs = np.arange(len(xlabels))    # locations for x-axis, [0, 1, 2, 3, 4, 5, 6]

	plt.figure(figsize=(10, 4.5))
	ax1 = plt.gca()

	ax1.set_xlabel(xlabel, fontsize=12)
	ax1.set_xticks(xs)
	ax1.set_xticklabels(xlabels, rotation=0, ha='center', fontsize=10)

	start = 0
	data_num = len(df_data.columns)
	if data_num % 2 == 0:
		start = + width / 2
	start = start - (data_num//2)*width

	for i in range(len(df_data.columns)):
		ax1.bar(xs + start + width* (i), df_data[df_data.columns[i]], width, label=df_data.columns[i])

	ax1.set_ylabel(ylabel, fontsize=12)

	plt.legend(loc='upper center', fontsize=10, framealpha=0.1, bbox_to_anchor=(0.5, -0.15)	, ncol=4, frameon=False)

	plt.title(title)
	plt.tight_layout()
	# plt.show()
	plt.savefig(save_path)


def draw_plot_2v2(
        data_l1, data_l1_label,
        data_l2, data_l2_label,
        data_r1, data_r1_label,
        data_r2, data_r2_label,
        xlabels,
        title='',
        xlabel='',
        ylabel_1='',
        ylabel_2='',
        save_path='output.png',
    ):
    width = 0.35

    xs = np.arange(len(data_l1))    # locations for x-axis, [0, 1, 2, 3, 4, 5, 6]

    plt.figure(figsize=(8, 4.5))
    ax1 = plt.gca()

    ax1.set_xlabel('Algorithm', fontsize=12)
    ax1.set_xticks(xs)
    ax1.set_xticklabels(xlabels, rotation=30, ha='right', fontsize=10)

    ax1.bar(xs - width/2, data_l1, width, label=data_l1_label, color='skyblue')
    ax1.bar(xs + width/2, data_l2, width, label=data_l2_label, color='lightcoral')
    ax1.set_ylabel(ylabel_1, fontsize=12)


    ax2 = ax1.twinx()
    ax2.plot(xs - width/2, data_r1, marker='o', linestyle='-', linewidth=1.5, markersize=8, label=data_r1_label, color='steelblue')
    ax2.plot(xs + width/2, data_r2, marker='s', linestyle='--', linewidth=1.5, markersize=8, label=data_r2_label, color='firebrick')
    ax2.set_ylabel(ylabel_2, fontsize=12)


    # combine legends
    lines1, labels1 = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    plt.legend(lines1 + lines2, labels1 + labels2, loc='upper left', fontsize=10, framealpha=0.5)

    plt.title(title)
    plt.xticks(rotation=45, ha='right')
    plt.tight_layout()
    # plt.show()
    plt.savefig(save_path)

def draw_plot_2v3(
        data_l1, data_l1_label,
        data_l2, data_l2_label,
        data_l3, data_l3_label,
        data_l4, data_l4_label,
        data_r1, data_r1_label,
        data_r2, data_r2_label,
        xlabels,
        title='',
        xlabel='',
        ylabel_1='',
        ylabel_2='',
        save_path='plots_reported.png',
    ):
    width = 0.2

    xs = np.arange(len(data_l1))    # locations for x-axis, [0, 1, 2, 3, 4, 5, 6]

    plt.figure(figsize=(8, 4.5))
    ax1 = plt.gca()

    ax1.set_xlabel('Algorithm', fontsize=12)
    ax1.set_xticks(xs)
    ax1.set_xticklabels(xlabels, rotation=30, ha='right', fontsize=10)

    ax1.bar(xs - width*1.5, data_l1, width, label=data_l1_label, color='skyblue')
    ax1.bar(xs - width/2, data_l2, width, label=data_l2_label, color='lightcoral')
    ax1.bar(xs + width/2, data_l3, width, label=data_l3_label, color='cornflowerblue')
    ax1.bar(xs + width*1.5, data_l4, width, label=data_l4_label, color='tomato')

    ax1.set_ylabel(ylabel_1, fontsize=12)


    ax2 = ax1.twinx()
    ax2.plot(xs - width, data_r1, marker='o', linestyle='-', linewidth=1.5, markersize=8, label=data_r1_label, color='steelblue')
    ax2.plot(xs + width, data_r2, marker='s', linestyle='--', linewidth=1.5, markersize=8, label=data_r2_label, color='firebrick')
    ax2.set_ylabel(ylabel_2, fontsize=12)


    # combine legends
    lines1, labels1 = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    plt.legend(lines1 + lines2, labels1 + labels2, loc='upper left', fontsize=10, framealpha=0.3)

    plt.title(title)
    plt.xticks(rotation=45, ha='right')
    plt.tight_layout()
    # plt.show()
    plt.savefig(save_path)


if __name__ == '__main__':
    main()