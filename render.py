from math import sqrt, erf, log, exp
import io
import collections


def prob_normal_positive(mean, sigma):
    if sigma == 0:
        if mean > 0:
            return 1
        elif mean < 0:
            return 0
        else:
            return 0.5
    p = 0.5 * (1 + erf(mean / (sqrt(2) * sigma)))
    return p


class Distribution(object):
    def __init__(self):
        self.n = 0
        self.sum = 0
        self.sum2 = 0
        self.max = float('-inf')
        self.min = float('+inf')

    def add_value(self, x):
        self.n += 1
        self.sum += x
        self.sum2 += x*x
        self.max = max(self.max, x)
        self.min = min(self.min, x)

    def mean(self):
        if self.n == 0:
            return 0
        return self.sum / self.n

    def sigma(self):
        if self.n < 2:
            return 0
        mean = self.mean()
        sigma2 = (self.sum2 - 2*mean*self.sum + mean*mean*self.n) / (self.n - 1)
        if sigma2 < 0:  # numerical errors
            sigma2 = 0
        return sqrt(sigma2)

    def to_html(self):
        if self.n == 0:
            return '--'
        if self.sigma() < 1e-10:
            return '{:.3f}'.format(self.mean())
        return (
            '<span title="{}..{}, {} items">'
            '{:.3f} &plusmn; <i>{:.3f}</i>'
            '</span>'.format(
                self.min, self.max, self.n, self.mean(), self.sigma()))

    def prob_mean_larger(self, other):
        """
        Probability that actual mean of this dist is larger than of another.
        """
        if self.n == 0 or other.n == 0:
            return 0.5
        diff_mean = self.mean() - other.mean()

        sigma1 = self.sigma()
        sigma2 = other.sigma()
        # If we have no data about variance of one of the distributions,
        # we take it from another distribution.
        if other.n == 1:
            sigma2 = sigma1
        if self.n == 1:
            sigma1 = sigma2

        diff_sigma = sqrt(sigma1**2 + sigma2**2)

        return prob_normal_positive(diff_mean, diff_sigma)


def color_prob(p):
    if p < 0.5:
        red = 1 - 2 * p;
        return '#{:x}00'.format(int(15 * red))
    else:
        green = 2 * p - 1
        return '#0{:x}0'.format(int(15 * green))


def render_table(results, baseline_results):
    result_by_seed = {
        result['seed']: result for result in results}
    baseline_result_by_seed = {
        result['seed']: result for result in baseline_results}

    all_seeds = sorted(result_by_seed.keys() | baseline_result_by_seed.keys())

    groups = [(seed, [seed]) for seed in all_seeds]
    groups = [('all', all_seeds)] + groups

    fout = io.StringIO()
    fout.write('<table>')

    for group_name, group in groups:
        fout.write('<tr>')
        fout.write('<td>{}</td>'.format(group_name))

        d = Distribution()
        for seed in group:
            result = result_by_seed.get(seed)
            if result is None:
                continue
            d.add_value(result['total_time'])
        fout.write('<td>time = {}</td>'.format(d.to_html()))

        d = Distribution()
        for seed in group:
            result = result_by_seed.get(seed)
            if result is None:
                continue
            d.add_value(result['score'])
        fout.write('<td>score = {}</td>'.format(d.to_html()))

        d = Distribution()
        for seed in group:
            result = result_by_seed.get(seed)
            baseline_result = baseline_result_by_seed.get(seed)
            if result is None or baseline_result is None:
                continue
            d.add_value(result['score'] - baseline_result['score'])
        if d.n > 0:
            fout.write('<td>delta = {}</td>'.format(d.to_html()))

        fout.write('</tr>')

    fout.write('</table>')
    return fout.getvalue()
