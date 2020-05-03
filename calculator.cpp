#include <iostream>
#include <cmath>
#include <string>
#include <sstream>
#include <fstream>
#include <utility>
using namespace std;
typedef pair<double, short int> pdi;

//Base rate of a 5* pull; 4% normally, 6% during Gala
#define BASE 0.04
//Base rate of the featured units; 0.5% per adventurer, 0.8% per dragon
#define RATE 0.01 //this currently accounts for 2 target adventurers

/* ft_prob computes the probability of obtaining a featured unit.
 * this takes into account rate-up mechanics, which takes in how many
 * summons you've done in r_up. For every 10 summons in r_up, the total rate
 * increases by 0.5%. We assume this 0.5% is distributed equally across all
 * units, so the rate of each adventurer is multiplied by
 *
 * (BASE + pity) / BASE.
 */
 //note that r_up counts the number of summons counting toward pity, not the rate itself.
double ft_prob(int r_up){
	double multiplier = 1.0 + ((r_up / 10) * 0.005) / BASE;
	return multiplier * RATE;
}

/* nft_prob computes the probability getting a non-featured 5* unit.
 * This basically just takes the total current rate as
 *
 * BASE + ((r_up / 10) * 0.005),
 *
 * and subtracts the rate of getting a featured unit.
 */
double nft_prob(int r_up)
{
	return BASE + ((r_up / 10) * 0.005) - ft_prob(r_up);
}

/* n_prob computes the probability of not getting a 5* in one summon.
 * This takes the total current 5* rate, and subtracts it from 100%.
 */
double n_prob(int r_up)
{
	return 1.0 - (BASE + ((r_up / 10) * 0.005));
}

/* ft_guaranteed_prob computes the probability of getting featured if capped rate at 100 summons.
 * in this case, the next summon is guaranteed to be 5*, and we assume each adventurer's rate is
 * scaled by the same amount.
 */
double ft_guaranteed_prob()
{
	return RATE / BASE;
}

/* tfold_ft_prob computes the probability of obtaining a featured 5* on a tenfold.
 * To do this, we use complementary counting, and we compute the probability
 * that we _don't_ obtain a featured unit. The probability of not obtaining a featured
 * unit is
 *
 * (1 - rate)^10
 *
 * which we then subtract from 1. This is the standard computation; if there is a guaranteed summon,
 * we make the assumption that the remaining 9 summons will still be computed with the same pity rate.
 */
double tfold_ft_prob(int r_up)
{
	if (r_up >= 100)
		return 1.0 - ((1.0 - ft_guaranteed_prob()) * pow(1.0 - ft_prob(r_up), 9));
	return 1.0 - pow(1.0 - ft_prob(r_up), 10);
}

/* tfold_nft_prob computes the probability of achieving at least 1 and only nonfeatured 5* on tenfold.
 * To do this, we compute the probability of achieving at least one 5*, given by
 *
 * 1 - (not_5*_rate)^10
 *
 * and subtract the probability that a featured is obtained. If there is a guaranteed summon,
 * we compute the probability it is not a featured.
 */
double tfold_nft_prob(int r_up)
{
	if (r_up >= 100)
		return 1.0 - tfold_ft_prob(r_up);
	return (1.0 - pow(n_prob(r_up), 10)) - tfold_ft_prob(r_up);
}

/* tfold_n_prob computes the probability of achieving no 5* on tenfold.
 * 
 * (not_5*_rate)^10
 *
 * In the special case where we have a guaranteed 5*, this probability is 0.
 */
double tfold_n_prob(int r_up)
{
	if (r_up >= 100)
		return 0.0;
	return pow(n_prob(r_up), 10);
}

/* The probability function recursively calculates the probability of achieving any featured 5* unit.
 * It computes the different probability branches when using a single summon.
 *
 * single = probability of obtaining a featured on a single summon
 * single_reset = probability of obtaining a nonfeatured 5* on a single summon
 * single_none = no 5*.
 *
 * tenfold, tenfold_reset, and tenfold_none are similarly defined. When using a single, we traverse the
 * probability branches.
 *
 * single: you've obtained the featured 5*, so we just add this probability
 * single_reset: pity rate is now 0, so if this happens, the remaining probability is recursively comptued
 *				 by reducing the number of summons by 1 and reseting pity rate to 0.
 * single_none: no 5* unit was obtained. The remaining probability is recursively computed by reducing the
 *				number of summons by 1 and increasing the pity summons by 1.
 *
 * We do similar computations for tenfold_success. Then, we record which of the two, doing a single or doing a tenfold,
 * is probabilistically better for summoning.
 *
 * The remaining fluff in this code is for memoization, to reduce the normally exponential running time to polynomial.*/
double probability(int summons, int r_up, pdi **memo)
{
	if (summons <= 0 || r_up >= 110) return 0.0;
	if (memo[r_up][summons].first >= 0.0) return memo[r_up][summons].first;
	double single = ft_prob(r_up);
	double single_reset = nft_prob(r_up);
	double single_none = n_prob(r_up);
	double tenfold = tfold_ft_prob(r_up);
	double tenfold_reset = tfold_nft_prob(r_up);
	double tenfold_none = tfold_n_prob(r_up);

	double single_success = single + single_reset * probability(summons - 1, 0, memo) + single_none * probability(summons - 1, r_up + 1, memo);
	double tenfold_success = tenfold + tenfold_reset * probability(summons - 10, 0, memo) + tenfold_none * probability(summons - 10, r_up + 10, memo);

	if (summons < 10 || single_success > tenfold_success)
	{
		memo[r_up][summons] = { single_success, -1 };
		return single_success;
	}
	memo[r_up][summons] = { tenfold_success, 1 };
	return memo[r_up][summons].first;
}

int main()
{
	int tot_summons = 1001;
	pdi **memo = new pdi*[110];
	for (int i = 0; i < 110; i++)
	{
		memo[i] = new pdi[tot_summons];
		for (int j = 0; j < tot_summons; j++)
		{
			memo[i][j] = {-1.0, 0};
		}
	}
	cout << probability(1000, 0, memo) << endl;
	stringstream ss;
	ss << ",";
	for (int i = 0; i < tot_summons; i++)
	{
		ss << i << ",";
	}
	ss << "\n";
	for (int j = 0; j < 110; j++)
	{
		ss << j << ",";
		for (int i = 0; i < tot_summons; i++)
		{
			ss << memo[j][i].first * memo[j][i].second << ",";
		}
		ss << "\n";
	}
	ofstream output("output1.csv");
	output << ss.str();
	return 0;
}
