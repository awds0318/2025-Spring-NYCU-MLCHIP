#include "Pattern.h"

bool compare(const pair<double, int>& a, const pair<double, int>& b) {
    return a.first > b.first; // 這樣可以從大到小排序
}

void Pattern::run()
{
	if (cycle == 0)
	{	
		// cout << "Pattern reset" << endl;

		rst.write(1);
		in_valid.write(0);
	}
	else if (cycle == 1)
	{
		rst.write(0);
		ifstream inputFile(("data/" + img_name).c_str());

		double img_element;
		int cnt{0};
		vector<double> numbers;

		while (inputFile >> img_element)
		{
			img[cnt] = (double)(img_element);
			cnt++;
		}

		in_valid.write(1);
	}
	else if (out_valid.read() == 1)
	{
		ifstream class_file("data/imagenet_classes.txt");
		vector<string> class_name;
		string class_name_element;
		while (getline(class_file, class_name_element))
		{
			class_name.push_back(class_name_element);
		}

		vector<pair<double, int> > indexed_values;

		for (int i = 0; i < 1000; ++i) 
		{
			indexed_values.push_back(make_pair(output_softmax[i].read(), i));
		}

		sort(indexed_values.begin(), indexed_values.end(), compare);

		cout << "Top 100 classes:" << endl;
		cout << "=================================================" << endl;
		cout << fixed << setprecision(2);
		cout << right << setw(5) << "idx"
			 << " | " << setw(8) << "val"
			 << " | " << setw(11) << "possibility" 
			 << " | " << "class name" << endl;
		cout << "-------------------------------------------------" << endl;

		for (int i = 0; i < 100; ++i)
		{
			cout << right << setw(5) << indexed_values[i].second
				 << " | " << setw(8) << double(output_linear[indexed_values[i].second].read())
				 << " | " << setw(11) << (indexed_values[i].first * 100)
				 << " | " << class_name[indexed_values[i].second] << endl;
		}
	
	cout << "=================================================" << endl;

	}
	else
	{
		in_valid.write(0);
	}

	// cout << "Pattern cycle: " << cycle << endl;
	cycle++;
	if (cycle == CYCLE)
		exit(0);
	// sc_stop();
}
