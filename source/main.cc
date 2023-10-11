// ----------------------------------------------------------------------------
//
//  Copyright (C) 2003-2013 Fons Adriaensen <fons@linuxaudio.org>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include <clthreads.h>
#include <dlfcn.h>
#include "audio.h"
#include "imidi.h"
#include "model.h"
#include "slave.h"
#include "iface.h"


#ifdef __linux__
static const char *options = "htuAJBM:N:S:I:W:d:r:p:n:s:";
#else
static const char *options = "htuJBM:N:S:I:W:s:";
#endif
static char  optline [1024];
static bool  t_opt = false;
static bool  u_opt = false;
static bool  A_opt = false;
static bool  B_opt = false;
static int   r_val = 48000;
static int   p_val = 1024;
static int   n_val = 2;
static const char *N_val = "aeolus";
static const char *S_val = "stops";
static const char *I_val = "Aeolus";
static const char *W_val = "waves";
static const char *d_val = "default";
static const char *s_val = 0;
static Lfq_u32  note_queue (256);
static Lfq_u32  comm_queue (256);
static Lfq_u8   midi_queue (1024);
static Iface   *iface;


static void help (void)
{
    fprintf (stderr, "\nAeolus %s\n\n",VERSION);
    fprintf (stderr, "  (C) 2003-2013 Fons Adriaensen  <fons@linuxaudio.org>\n");
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "  -h                 Display this text\n");
    fprintf (stderr, "  -t                 Text mode user interface\n");
    fprintf (stderr, "  -u                 Use presets file in user's home dir\n");
#ifdef __linux__
    fprintf (stderr, "  -N <name>          Name to use as JACK and ALSA client [aeolus]\n");   
#else
    fprintf (stderr, "  -N <name>          Name to use as JACK and CoreMIDI client [aeolus]\n");   
#endif
    fprintf (stderr, "  -S <stops>         Name of stops directory [stops]\n");   
    fprintf (stderr, "  -I <instr>         Name of instrument directory [Aeolus]\n");   
    fprintf (stderr, "  -W <waves>         Name of waves directory [waves]\n");   
    fprintf (stderr, "  -J                 Use JACK (default), with options:\n");
    fprintf (stderr, "    -s               Select JACK server\n");
    fprintf (stderr, "    -B               Ambisonics B format output\n");
#ifdef __linux__
    fprintf (stderr, "  -A                 Use ALSA, with options:\n");
    fprintf (stderr, "    -d <device>        Alsa device [default]\n");
    fprintf (stderr, "    -r <rate>          Sample frequency [48000]\n");
    fprintf (stderr, "    -p <period>        Period size [1024]\n");
    fprintf (stderr, "    -n <nfrags>        Number of fragments [2]\n\n");
#endif
    exit (1);
}


static void procoptions (int ac, char *av [], const char *where)
{
    int k;
    
    optind = 1;
    opterr = 0;
    while ((k = getopt (ac, av, options)) != -1)
    {
        if (optarg && (*optarg == '-'))
        {
            fprintf (stderr, "\n%s\n", where);
	    fprintf (stderr, "  Missing argument for '-%c' option.\n", k); 
            fprintf (stderr, "  Use '-h' to see all options.\n");
            exit (1);
        }
	switch (k)
	{
        case 'h' : help (); exit (0);
 	case 't' : t_opt = true;  break;
 	case 'u' : u_opt = true;  break;
 	case 'A' : A_opt = true;  break;
	case 'J' : A_opt = false; break;
	case 'B' : B_opt = true; break;
        case 'r' : r_val = atoi (optarg); break;
        case 'p' : p_val = atoi (optarg); break;
        case 'n' : n_val = atoi (optarg); break;
        case 'N' : N_val = optarg; break; 
        case 'S' : S_val = optarg; break; 
        case 'I' : I_val = optarg; break; 
        case 'W' : W_val = optarg; break; 
        case 'd' : d_val = optarg; break; 
	case 's' : s_val = optarg; break;
        case '?':
            fprintf (stderr, "\n%s\n", where);
            if (optopt != ':' && strchr (options, optopt)) fprintf (stderr, "  Missing argument for '-%c' option.\n", optopt); 
            else if (isprint (optopt)) fprintf (stderr, "  Unknown option '-%c'.\n", optopt);
            else fprintf (stderr, "  Unknown option character '0x%02x'.\n", optopt & 255);
            fprintf (stderr, "  Use '-h' to see all options.\n");
            exit (1);
        default:
            abort ();
 	}
    }
}


static int readconfig (const char *path)
{
    FILE *F;
    char  s [1024];
    char *p;
    char *av [30];
    int   ac;

    av [0] = 0;
    ac = 1;
    if ((F = fopen (path, "r")))
    {
        while (fgets (optline, 1024, F) && (ac == 1))
        {
 	    p = strtok (optline, " \t\n");
            if (*p == '#') continue;
            while (p && (ac < 30))
	    {
		av [ac++] = p;
                p = strtok (0, " \t\n");
	    }
	}
	fclose (F);
        sprintf (s, "In file '%s':", path);
        procoptions (ac, av, s);
        return 0;
    }
    return 1;
}


static void sigint_handler (int)
{
    signal (SIGINT, SIG_IGN);
    iface->stop ();
}


int main (int ac, char *av [])
{
    ITC_ctrl       itcc;
    Audio         *audio;
    Imidi         *imidi;
    Model         *model;
    Slave         *slave;
    void          *so_handle;
    iface_cr      *so_create;
    char           s [1024];
    char          *p;
    int            n;
     
    p = getenv ("HOME");
    if (p) sprintf (s, "%s/.aeolusrc", p);
    else strcpy (s, ".aeolusrc");
    if (readconfig (s)) readconfig ("/etc/aeolus.conf"); 
    procoptions (ac, av, "On command line:");

    if (mlockall (MCL_CURRENT | MCL_FUTURE)) fprintf (stderr, "Warning: memory lock failed.\n");

    if (t_opt) sprintf (s, "%s/aeolus_txt.so", LIBDIR);
    else       sprintf (s, "%s/aeolus_x11.so", LIBDIR);
    so_handle = dlopen (s, RTLD_NOW);
    if (! so_handle)
    {
        fprintf (stderr, "Error: can't open user interface plugin: %s.\n", dlerror ());
        return 1;
    }
    so_create = (iface_cr *) dlsym (so_handle, "create_iface");
    if (! so_create)
    {
        fprintf (stderr, "Error: can't create user interface plugin: %s.\n", dlerror ());
        dlclose (so_handle);
        return 1;
    }

    audio = new Audio (N_val, &note_queue, &comm_queue);
#ifdef __linux__
    if (A_opt) audio->init_alsa (d_val, r_val, p_val, n_val);
    else       audio->init_jack (s_val, B_opt, &midi_queue);
#else
    audio->init_jack (s_val, B_opt, &midi_queue);
#endif
    model = new Model (&comm_queue, &midi_queue, audio->midimap (), audio->appname (), S_val, I_val, W_val, u_opt);
    imidi = new Imidi (&note_queue, &midi_queue, audio->midimap (), audio->appname ());
    slave = new Slave ();
    iface = so_create (ac, av);

    ITC_ctrl::connect (audio, EV_EXIT,  &itcc, EV_EXIT);    
    ITC_ctrl::connect (audio, EV_QMIDI, model, EV_QMIDI);    
    ITC_ctrl::connect (audio, TO_MODEL, model, FM_AUDIO);    
    ITC_ctrl::connect (imidi, EV_EXIT,  &itcc, EV_EXIT);    
    ITC_ctrl::connect (imidi, EV_QMIDI, model, EV_QMIDI);    
    ITC_ctrl::connect (imidi, TO_MODEL, model, FM_IMIDI);    
    ITC_ctrl::connect (model, EV_EXIT,  &itcc, EV_EXIT);    
    ITC_ctrl::connect (model, TO_AUDIO, audio, FM_MODEL);    
    ITC_ctrl::connect (model, TO_SLAVE, slave, FM_MODEL);    
    ITC_ctrl::connect (model, TO_IFACE, iface, FM_MODEL);    
    ITC_ctrl::connect (slave, EV_EXIT,  &itcc, EV_EXIT);    
    ITC_ctrl::connect (slave, TO_AUDIO, audio, FM_SLAVE);    
    ITC_ctrl::connect (slave, TO_MODEL, model, FM_SLAVE);    
    ITC_ctrl::connect (iface, EV_EXIT,  &itcc, EV_EXIT);    
    ITC_ctrl::connect (iface, TO_MODEL, model, FM_IFACE);    

    audio->start ();
    if (imidi->thr_start (SCHED_FIFO, audio->relpri () - 20, 0x00010000))
    {
        fprintf (stderr, "Warning: can't run midi thread in RT mode.\n");
	imidi->thr_start (SCHED_OTHER, 0, 0x00010000);
    }
    if (model->thr_start (SCHED_FIFO, audio->relpri () - 30, 0x00010000))
    {
        fprintf (stderr, "Warning: can't run model thread in RT mode.\n");
	model->thr_start (SCHED_OTHER, 0, 0x00010000);
    }
    slave->thr_start (SCHED_OTHER, 0, 0x00010000);
    iface->thr_start (SCHED_OTHER, 0, 0x00020000);

    signal (SIGINT, sigint_handler); 
    n = 4;
    while (n)
    {    
	itcc.get_event (1 << EV_EXIT);
	{
	    if (n-- == 4)
	    {   
                imidi->terminate ();
		model->terminate ();
		slave->terminate ();
		iface->terminate ();
	    }
	}
    }

    delete audio;
    delete imidi;
    delete model;
    delete slave;
    delete iface;
    dlclose (so_handle);
 
    return 0;
}

