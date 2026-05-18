#include <stdlib.h>
#include <m_pd.h>
#include "Python.h"
#include "m_pyo.h"

static t_class *pyo_tilde_class;

typedef struct _pyo_tilde {
    t_object  obj;
    t_sample  f;
    int debug;
    int bs;
    int add;
    int ichnls;
	int ochnls;
    float sr;
	int stdout_set;
	int stderr_set;
    const char *file;
    t_sample **in;
    t_sample **out;
	t_outlet *stdout;
	t_atom *stdout_vec;
    int id;                 /* pyo server id */
    float *inbuf;           /* pyo input buffer */
    float *outbuf;          /* pyo output buffer */
    char *msg;              /* preallocated string to construct message for pyo */
    void (*callback)(int);  /* pointer to pyo embedded server callback */
    PyThreadState *interp;  /* Python thread state linked to this sub interpreter */
} t_pyo_tilde;

t_int *pyo_tilde_perform(t_int *w) {
    int i, j, n;
    t_pyo_tilde *x = (t_pyo_tilde *)(w[1]); /* pointer to instance struct */
    n = (int)(w[2]);                        /* vector size */
    t_sample **in = x->in;
    t_sample **out = x->out;
    for (i=0; i<n; i++) {
        for (j=0; j<x->ichnls; j++) {
            x->inbuf[i*x->ichnls+j] = in[j][i];
        }
    }
    (*x->callback)(x->id);
    for (i=0; i<n; i++) {
        for (j=0; j<x->ochnls; j++) {
            out[j][i] = x->outbuf[i*x->ochnls+j];
        }
    }
    return (w+3);
}

void pyo_tilde_dsp(t_pyo_tilde *x, t_signal **sp) {
    int i, err;
    t_sample **dummy = x->in;
    for (i=0; i<x->ichnls; i++)
        *dummy++ = sp[i]->s_vec;
    dummy = x->out;
    for (i=x->ichnls; i<x->ichnls+x->ochnls; i++)
        *dummy++ = sp[i]->s_vec;
    /* reset pyo only if sampling rate or buffer size have changed */
    if ((float)sp[0]->s_sr != x->sr || (int)sp[0]->s_n != x->bs) {
        x->sr = (float)sp[0]->s_sr;
        x->bs = (int)sp[0]->s_n;
        pyo_set_server_params(x->interp, x->sr, x->bs);
        if (x->file != NULL) {
            err = pyo_exec_file(x->interp, x->file, x->msg, x->add, x->debug);
            if (err == 1) {
                post("Unable to open file < %s >", x->file);
                x->file = NULL;
            } else if (err == 2) {
                pd_error(x, x->msg);
                x->file = NULL;
            }
        }
    }
    dsp_add(pyo_tilde_perform, 2, x, sp[0]->s_n);
}

static void pyo_load_file(t_pyo_tilde *x) {
    int err;
    if (pyo_is_server_started(x->interp)) {
        err = pyo_exec_file(x->interp, x->file, x->msg, x->add, x->debug);
        if (err == 1) {
            post("Unable to open file < %s >", x->file);
            x->file = NULL;
        } else if (err == 2) {
			pd_error(x, x->msg);
            x->file = NULL;
        }
    }
}

static void pyo_tilde_free(t_pyo_tilde *x) {
    freebytes(x->in, sizeof(x->in));
    freebytes(x->out, sizeof(x->out));
    freebytes(x->msg, sizeof(x->msg));
    pyo_end_interpreter(x->interp);
	if (x->stdout_set)
		outlet_free(x->stdout);
}

static void pyo_tilde_set_value(t_pyo_tilde *x, char *att, int argc, t_atom *argv) {
    int err, bracket = 0;
    char fchar[32];
    t_symbol *c = atom_getsymbol(argv);
    argc--; argv++;
    sprintf(x->msg, "%s%s=", c->s_name, att);
    if (argc > 1) {
        strcat(x->msg, "[");
        bracket = 1;    
    }
    while (argc-- > 0) {
        if (argv->a_type == A_SYMBOL) {
            strcat(x->msg, atom_getsymbol(argv)->s_name);
        }
        else if (argv->a_type == A_FLOAT) {
            sprintf(fchar, "%.6f", atom_getfloat(argv));
            strcat(x->msg, fchar);
        }
        if (argc > 0)
            strcat(x->msg, ",");
        argv++;
    }
    if (bracket)
        strcat(x->msg, "]");
    err = pyo_exec_statement(x->interp, x->msg, x->debug);
    if (err)
        pd_error(x, x->msg);
}

static void pyo_tilde_value(t_pyo_tilde *x, t_symbol *s, int argc, t_atom *argv) {
    char *att = ".value";
	(void)(s); /* unused */
    pyo_tilde_set_value(x, att, argc, argv);
}

static void pyo_tilde_set(t_pyo_tilde *x, t_symbol *s, int argc, t_atom *argv) {
    char *att = "";
	(void)(s); /* unused */
    pyo_tilde_set_value(x, att, argc, argv);
}

static void pyo_tilde_create(t_pyo_tilde *x, t_symbol *s, int argc, t_atom *argv) {
    int err;
    const char *varname, *object;
    char fchar[32];
    t_symbol *c = atom_getsymbol(argv);
	(void)(s); /* unused */
    varname = c->s_name;
    argc--; argv++;
    c = atom_getsymbol(argv);
    object = c->s_name;
    argc--; argv++;
    sprintf(x->msg, "%s=%s(", varname, object);
    while (argc-- > 0) {
        if (argv->a_type == A_SYMBOL) {
            strcat(x->msg, atom_getsymbol(argv)->s_name);
        }
        else if (argv->a_type == A_FLOAT) {
            sprintf(fchar, "%f", atom_getfloat(argv));
            strcat(x->msg, fchar);
        }
        if (argc > 0)
            strcat(x->msg, ",");
        argv++;
    }
    strcat(x->msg, ")");
    err = pyo_exec_statement(x->interp, x->msg, x->debug);
    if (err)
        pd_error(x, x->msg);
}

static int pyo_tilde_get_stdout(t_pyo_tilde *x)
{
	int counter = 0;
	char *msg;
	while (pyo_dequeue_stdout(&msg)) {
		if (counter == 0) sprintf(x->msg, msg);
		else strcat(x->msg, msg);
		free(msg);
		counter++;
	}
	return counter;
}

static void pyo_tilde_exec(t_pyo_tilde *x, t_symbol *s, int argc, t_atom *argv) {
    int err, i;
    char fchar[32];
	(void)(s); /* unused */
    for (i = 0; i < argc; i++) {
        if (argv->a_type == A_SYMBOL) {
			if (i == 0) sprintf(x->msg, atom_getsymbol(argv)->s_name);
			else strcat(x->msg, atom_getsymbol(argv)->s_name);
        }
        else if (argv->a_type == A_FLOAT) {
            sprintf(fchar, "%f", atom_getfloat(argv));
			if (i == 0) sprintf(x->msg, fchar);
			else strcat(x->msg, fchar);
        }
        if (argc > 0)
            strcat(x->msg, " ");
        argv++;
    }
    err = pyo_exec_statement(x->interp, x->msg, x->debug);
    if (err) {
		/* remove the newline character at the end of STDOUT
		 * by placing the null terminating character at its position */
		x->msg[strlen(x->msg)-1] = '\0';
		if (x->stderr_set) {
			t_atom *stdout_vec = x->stdout_vec;
			SETSYMBOL(stdout_vec, gensym(x->msg));
			outlet_anything(x->stdout, gensym("stderr"), 1, stdout_vec);
		}
		else {
			pd_error(x, x->msg);
		}
	}
	else if (pyo_tilde_get_stdout(x)){
		/* remove the newline character at the end of STDOUT
		 * by placing the null terminating character at its position */
		x->msg[strlen(x->msg)-1] = '\0';
		if (x->stdout_set) {
			t_atom *stdout_vec = x->stdout_vec;
			SETSYMBOL(stdout_vec, gensym(x->msg));
			outlet_anything(x->stdout, gensym("stdout"), 1, stdout_vec);
		}
		else {
			post(x->msg);
		}
	}
}

static void pyo_tilde_midi_event(t_pyo_tilde *x, t_symbol *s, int argc, t_atom *argv) {
    int status = 0, data1 = 0, data2 = 0;
	(void)(s); /* unused */
    if (argc > 0)
        status = (int)atom_getfloat(argv);
    if (argc > 1)
        data1 = (int)atom_getfloat(++argv);
    if (argc > 2)
        data2 = (int)atom_getfloat(++argv);
    pyo_add_midi_event(x->interp, status, data1, data2);
}

static void pyo_tilde_bpm(t_pyo_tilde *x, t_float f) {
	if (f < 0) {
		pd_error(x, "BPM value can't be negative");
		return;
	}
	pyo_set_bpm(x->interp, (double)f);
}

static void pyo_tilde_clear(t_pyo_tilde *x) {
    pyo_server_reboot(x->interp);
}

static void pyo_tilde_debug(t_pyo_tilde *x, t_float debug) {
    x->debug = debug <= 0 ? 0 : 1;
}

static void pyo_tilde_read(t_pyo_tilde *x, t_symbol *s, int argc, t_atom *argv) {
	(void)(s); /* unused */
    switch (argc) {
        case 1:
            x->add = 0;
            x->file = atom_getsymbol(argv)->s_name;
            break;
        case 2:
            x->add = strcmp(atom_getsymbol(argv++)->s_name, "-a") == 0 ? 1 : 0;
            x->file = atom_getsymbol(argv)->s_name;
            break;
    }
	pyo_load_file(x);
}

static void pyo_tilde_call(t_pyo_tilde *x, t_symbol *s, int argc, t_atom *argv) {
    int err;
    char fchar[32];
	(void)(s); /* unused */
    sprintf(x->msg, "%s(", atom_getsymbol(argv)->s_name);
    argc--; argv++;
    while (argc-- > 0) {
        if (argv->a_type == A_SYMBOL) {
            strcat(x->msg, atom_getsymbol(argv)->s_name);
        }
        else if (argv->a_type == A_FLOAT) {
            sprintf(fchar, "%f", atom_getfloat(argv));
            strcat(x->msg, fchar);
        }
        if (argc > 0)
            strcat(x->msg, ", ");
        argv++;
    }
    strcat(x->msg, ")");
    err = pyo_exec_statement(x->interp, x->msg, x->debug);
    if (err)
        pd_error(x, x->msg);
}

static void *pyo_tilde_new(t_symbol *s, int argc, t_atom *argv) {
    int i;
	int ichnls_set = 0;
	int ochnls_set = 0;
	int load_file = 0;
	int setting_debug = 0;
	int loading_file = 0;
	int debug_set = 0;
	int setting_stdout = 0;
	int setting_stderr = 0;
	t_symbol *debug_flag = gensym("-debug");
	t_symbol *load_flag = gensym("-load");
	t_symbol *stdout_flag = gensym("-stdout");
	t_symbol *stderr_flag = gensym("-stderr");
	t_symbol *outlet_arg = gensym("outlet");
	t_symbol *console_arg = gensym("console");

    t_pyo_tilde *x = (t_pyo_tilde *)pd_new(pyo_tilde_class);
	(void)(s); /* unused */

	x->stdout_set = x->stderr_set = 0;

	for (i = 0; i < argc; i++) {
		if (argv->a_type == A_FLOAT) {
			if (setting_debug) {
				x->debug = (int)atom_getfloat(argv);
				setting_debug = 0;
				debug_set = 1;
			}
			else if (loading_file) {
				pd_error(x, "after -load a python file path should be provided");
				loading_file = 0;
				return (void *)x;
			}
			else {
				if (i == 0) {
					x->ichnls = (int)atom_getfloat(argv);
					ichnls_set = 1;
				}
				else {
					x->ochnls = (int)atom_getfloat(argv);
					ochnls_set = 1;
				}
			}
		}
		else if (argv->a_type == A_SYMBOL) {
			t_symbol *strarg = atom_gensym(argv);
			if (strarg == debug_flag) {
				setting_debug = 1;
			}
			else if (strarg == load_flag) {
				loading_file = 1;
			}
			else if (strarg == stdout_flag) {
				setting_stdout = 1;
			}
			else if (strarg == stderr_flag) {
				setting_stderr = 1;
			}
			else {
				if (loading_file) {
					x->add = 0;
            		x->file = strarg->s_name;
					load_file = 1;
					loading_file = 0;
				}
				else if (setting_stdout) {
					if (strarg == outlet_arg) {
						x->stdout_set = 1;
					}
					else if (strarg == console_arg) {
						x->stdout_set = 0;
					}
					else {
						pd_error(x, "unknown value to -stdout, must be \"outlet\" or \"console\"");
						return (void *)x;
					}
					setting_stdout = 0;
				}
				else if (setting_stderr) {
					if (strarg == outlet_arg) {
						x->stderr_set = 1;
					}
					else if (strarg == console_arg) {
						x->stderr_set = 0;
					}
					else {
						pd_error(x, "unknown value to -stderr, must be \"outlet\" or \"console\"");
						return (void *)x;
					}
					setting_stderr = 0;
				}
			}
		}
		argv++;
	}
	if (ichnls_set == 0) x->ichnls = 2;
	if (ochnls_set == 0) x->ochnls = x->ichnls;
    x->bs = 64;
    x->sr = 44100.0;
    if (!load_file) x->file = NULL;
    x->add = 0;
    if (!debug_set) x->debug = 0;
	if (x->stdout_set|| x->stderr_set) {
		/* vector for outputting lists out the last outlet */
		x->stdout_vec = (t_atom *)getbytes(sizeof(t_atom));
		x->stdout_vec[0].a_type = A_SYMBOL;
	}

    /* create signal inlets (first is done in pyo_tilde_setup) */
    for (i=1; i<x->ichnls; i++) {
        inlet_new(&x->obj, &x->obj.ob_pd, gensym("signal"), gensym("signal"));
	}
    /* create signal outlets */
    for (i=0; i<x->ochnls; i++) {
        outlet_new(&x->obj, gensym("signal"));
	}
	/* if set by the user, create a control outlet to route STDOUT and/or STDERR inside the patch */
	if (x->stdout_set || x->stderr_set) {
		x->stdout = outlet_new(&x->obj, 0);
	}

    x->in = (t_sample **)getbytes(x->ichnls * sizeof(t_sample **));
    x->out = (t_sample **)getbytes(x->ochnls * sizeof(t_sample **));
    x->msg = (char *)getbytes(262144 * sizeof(char *));

    for (i=0; i<x->ichnls; i++)
        x->in[i] = 0;

    for (i=0; i<x->ochnls; i++)
        x->out[i] = 0;

    x->interp = pyo_new_interpreter(x->sr, x->bs, x->ichnls, x->ochnls);
    
    x->inbuf = (float *)pyo_get_input_buffer_address(x->interp);
    x->outbuf = (float *)pyo_get_output_buffer_address(x->interp);
    x->callback = (void *)pyo_get_embedded_callback_address(x->interp);
    x->id = pyo_get_server_id(x->interp);

	/* dump the first stdout note about wdPython */
	pyo_tilde_get_stdout(x);

	if (load_file) pyo_load_file(x);

    return (void *)x;
}

void pyo_tilde_setup(void) {
    pyo_tilde_class = class_new(gensym("pyo~"), (t_newmethod)pyo_tilde_new,
        (t_method)pyo_tilde_free, sizeof(t_pyo_tilde), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(pyo_tilde_class, (t_method)pyo_tilde_dsp, gensym("dsp"), 0);
    class_addmethod(pyo_tilde_class, (t_method)pyo_tilde_clear, gensym("clear"), 0);
    class_addmethod(pyo_tilde_class, (t_method)pyo_tilde_value, gensym("value"), 
                    A_GIMME, 0); /* send value(s) to a Sig or SigTo object */
    class_addmethod(pyo_tilde_class, (t_method)pyo_tilde_set, gensym("set"), 
                    A_GIMME, 0); /* send value(s) to any object's attribute */
    class_addmethod(pyo_tilde_class, (t_method)pyo_tilde_read, gensym("read"), 
                    A_GIMME, 0); /* read a script file */
    class_addmethod(pyo_tilde_class, (t_method)pyo_tilde_call, gensym("call"), 
                    A_GIMME, 0); /* call a function or a method */
    class_addmethod(pyo_tilde_class, (t_method)pyo_tilde_create, gensym("create"), 
                    A_GIMME, 0); /* create a python object */
    class_addmethod(pyo_tilde_class, (t_method)pyo_tilde_midi_event, gensym("midi"), 
                    A_GIMME, 0); /* send a MIDI event */
    class_addmethod(pyo_tilde_class, (t_method)pyo_tilde_bpm, gensym("bpm"), 
                    A_DEFFLOAT, 0); /* set python's BPM */
    class_addmethod(pyo_tilde_class, (t_method)pyo_tilde_exec, gensym("exec"), 
                    A_GIMME, 0); /* execute a python statement */
    class_addmethod(pyo_tilde_class, (t_method)pyo_tilde_debug, gensym("debug"), 
                    A_DEFFLOAT, 0); /* set the debug (verbose) mode */
    CLASS_MAINSIGNALIN(pyo_tilde_class, t_pyo_tilde, f);
}
