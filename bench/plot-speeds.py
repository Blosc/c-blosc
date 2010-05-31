from pylab import *

linewidth=2
#markers= ['+', ',', 'o', '.', 's', 'v', 'x', '>', '<', '^']
#markers= [ 'x', '+', 'o', 's', 'v', '^', '>', '<', ]
markers= [ 's', 'o', 'v', '^', '+', 'x', '>', '<', '.', ',' ]
markersize = 8

def get_values(filename):
    f = open(filename)
    values = {}

    for line in f:
        if line.startswith('-->'):
            tmp = line.split('-->')[1]
            nthreads, size, elsize, sbits = [int(i) for i in tmp.split(', ')]
            # New run for nthreads
            (ratios, speedsw, speedsr) = ([], [], [])
            # Add a new entry for (ratios, speedw, speedr)
            values[nthreads] = (ratios, speedsw, speedsr)
            print "-->", nthreads, size, elsize, sbits
        elif line.startswith('memcpy(write):'):
            tmp = line.split(',')[1]
            memcpyw = float(tmp.split(' ')[1])
            print "memcpyw-->", memcpyw
        elif line.startswith('memcpy(read):'):
            tmp = line.split(',')[1]
            memcpyr = float(tmp.split(' ')[1])
            print "memcpyr-->", memcpyr
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
    #ylim(0, 10000)
    grid(True)

#     legends = [f[f.find('-'):f.index('.out')] for f in filenames]
#     legends = [l.replace('-', ' ') for l in legends]
    #legend([p[0] for p in plots], legends, loc = "upper left")
    legend([p[0] for p in plots], legends, loc = "best")


    #subplots_adjust(bottom=0.2, top=None, wspace=0.2, hspace=0.2)
    if outfile:
        savefig(outfile)
    else:
        show()

if __name__ == '__main__':

    import sys, getopt

    usage = """usage: %s [-o outfile] [-t title ] [-c] [-d] filename
 -o filename for output (only .png, .jpg  and .pdf extensions supported)
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
    dspeed = True
    yaxis = "No axis name"
    gtitle = "Please set a title!"

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

    if tit:
        gtitle = tit

    plots = []
    legends = []
    nthreads, values = get_values(filename)
    for nt in range(1, nthreads+1):
        print "Values for %s threads --> %s" % (nt, values)
        (ratios, speedw, speedr) = values[nt]
        if cspeed:
            speed = speedw
        else:
            speed = speedr
        #plot_ = semilogx(ratios, speed, linewidth=2)
        plot_ = plot(ratios, speed, linewidth=2)
        plots.append(plot_)
        nmarker = nt
        if nt > len(markers):
            nmarker = len(markers)
        setp(plot_, marker=markers[nmarker], markersize=markersize,
             linewidth=linewidth)
        legends.append("%d threads" % nt)

    show_plot(plots, yaxis, legends, gtitle)
