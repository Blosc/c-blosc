"""Script for plotting the results of the 'suite' benchmark.
Invoke without parameters for usage hints.

:Author: Francesc Alted
:Date: 2010-06-01
"""

import matplotlib as mpl
from pylab import *

KB_ = 1024
MB_ = 1024*KB_
GB_ = 1024*MB_
NCHUNKS = 128    # keep in sync with bench.c

linewidth=2
#markers= ['+', ',', 'o', '.', 's', 'v', 'x', '>', '<', '^']
#markers= [ 'x', '+', 'o', 's', 'v', '^', '>', '<', ]
markers= [ 's', 'o', 'v', '^', '+', 'x', '>', '<', '.', ',' ]
markersize = 8

def get_values(filename):
    f = open(filename)
    values = {"memcpyw": [], "memcpyr": []}

    for line in f:
        if line.startswith('-->'):
            tmp = line.split('-->')[1]
            nthreads, size, elsize, sbits = [int(i) for i in tmp.split(', ')]
            values["size"] = size * NCHUNKS / MB_;
            values["elsize"] = elsize;
            values["sbits"] = sbits;
            # New run for nthreads
            (ratios, speedsw, speedsr) = ([], [], [])
            # Add a new entry for (ratios, speedw, speedr)
            values[nthreads] = (ratios, speedsw, speedsr)
            #print "-->", nthreads, size, elsize, sbits
        elif line.startswith('memcpy(write):'):
            tmp = line.split(',')[1]
            memcpyw = float(tmp.split(' ')[1])
            values["memcpyw"].append(memcpyw)
        elif line.startswith('memcpy(read):'):
            tmp = line.split(',')[1]
            memcpyr = float(tmp.split(' ')[1])
            values["memcpyr"].append(memcpyr)
        elif line.startswith('comp(write):'):
            tmp = line.split(',')[1]
            speedw = float(tmp.split(' ')[1])
            ratio = float(line.split(':')[-1])
            speedsw.append(speedw)
            ratios.append(ratio)
        elif line.startswith('decomp(read):'):
            tmp = line.split(',')[1]
            speedr = float(tmp.split(' ')[1])
            speedsr.append(speedr)
            if "OK" not in line:
                print "WARNING!  OK not found in decomp line!"

    f.close()
    return nthreads, values


def show_plot(plots, yaxis, legends, gtitle):
    xlabel('Compresssion ratio')
    ylabel('Speed (MB/s)')
    title(gtitle)
    xlim(0, None)
    #ylim(0, 10000)
    ylim(0, None)
    grid(True)

#     legends = [f[f.find('-'):f.index('.out')] for f in filenames]
#     legends = [l.replace('-', ' ') for l in legends]
    #legend([p[0] for p in plots], legends, loc = "upper left")
    legend([p[0] for p in plots
            if not isinstance(p, mpl.lines.Line2D)],
           legends, loc = "best")


    #subplots_adjust(bottom=0.2, top=None, wspace=0.2, hspace=0.2)
    if outfile:
        print "Saving plot to:", outfile
        savefig(outfile)
    else:
        show()

if __name__ == '__main__':

    import sys, getopt

    usage = """usage: %s [-o outfile] [-t title ] [-c] [-d] filename
 -o filename for output (many extensions supported, e.g. .png, .jpg, .pdf)
 -t title of the plot
 -c plot compression speed
 -d plot decompression speed (default)
 \n""" % sys.argv[0]

    try:
        opts, pargs = getopt.getopt(sys.argv[1:], 'o:t:cd', [])
    except:
        sys.stderr.write(usage)
        sys.exit(0)

    progname = sys.argv[0]
    args = sys.argv[1:]

    # if we pass too few parameters, abort
    if len(pargs) < 1:
        sys.stderr.write(usage)
        sys.exit(0)

    # default options
    outfile = None
    tit = None
    cspeed = False
    gtitle = "Decompression speed"
    dspeed = True
    yaxis = "No axis name"

    # Get the options
    for option in opts:
        if option[0] == '-o':
            outfile = option[1]
        elif option[0] == '-t':
            tit = option[1]
        elif option[0] == '-c':
            cspeed = True
            dspeed = False
            gtitle = "Compression speed"
        elif option[0] == '-d':
            cspeed = False
            dspeed = True
            gtitle = "Decompression speed"

    filename = pargs[0]

    plots = []
    legends = []
    nthreads, values = get_values(filename)
    #print "Values:", values

    if tit:
        gtitle = tit
    else:
        gtitle += " (%(size).1f MB, %(elsize)d bytes, %(sbits)d bits)" % values

    for nt in range(1, nthreads+1):
        #print "Values for %s threads --> %s" % (nt, values[nt])
        (ratios, speedw, speedr) = values[nt]
        if cspeed:
            speed = speedw
        else:
            speed = speedr
        #plot_ = semilogx(ratios, speed, linewidth=2)
        plot_ = plot(ratios, speed, linewidth=2)
        plots.append(plot_)
        nmarker = nt
        if nt >= len(markers):
            nmarker = nt%len(markers)
        setp(plot_, marker=markers[nmarker], markersize=markersize,
             linewidth=linewidth)
        legends.append("%d threads" % nt)

    # Add memcpy lines
    if cspeed:
        mean = sum(values["memcpyw"]) / nthreads
        message = "memcpy (write to memory)"
    else:
        mean = sum(values["memcpyr"]) / nthreads
        message = "memcpy (read from memory)"
    plot_ = axhline(mean, linewidth=3, linestyle='-.', color='black')
    text(4.0, mean+50, message)
    plots.append(plot_)
    show_plot(plots, yaxis, legends, gtitle)


