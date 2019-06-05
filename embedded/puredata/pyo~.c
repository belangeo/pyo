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
    int chnls;
    float sr;
    const char *file;
    t_sample **in;
    t_sample **out;
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
        for (j=0; j<x->chnls; j++) {
            x->inbuf[i*x->chnls+j] = in[j][i];
        }
    }
    (*x->callback)(x->id);
    for (i=0; i<n; i++) {
        for (j=0; j<x->chnls; j++) {
            out[j][i] = x->outbuf[i*x->chnls+j];
        }
    }
    return (w+3);
}

void pyo_tilde_dsp(t_pyo_tilde *x, t_signal **sp) {
    int i, err;
    t_sample **dummy = x->in;
    for (i=0; i<x->chnls; i++)
        *dummy++ = sp[i]->s_vec;
    dummy = x->out;
    for (i=x->chnls; i<x->chnls*2; i++)
        *dummy++ = sp[i]->s_vec;
    /* reset pyo only if sampling rate or buffer size have changed */
    if ((float)sp[0]->s_sr != x->sr || (int)sp[0]->s_n != x->bs) {
        x->sr = (float)sp[0]->s_sr;
        x->bs = (int)sp[0]->s_n;
        pyo_set_server_params(x->interp, x->sr, x->bs);
        if (x->file != NULL) {
            err = pyo_exec_file(x->interp, x->file, x->msg, x->add);
            if (err == 1) {
                post("Unable to open file < %s >", x->file);
                x->file = NULL;
            } else if (err == 2) {
                post("Bad code in file < %s >", x->file);
                x->file = NULL;
            }
        }
    }
    dsp_add(pyo_tilde_perform, 2, x, sp[0]->s_n);
}

void *pyo_tilde_new(t_floatarg f) {
    int i;
    t_pyo_tilde *x = (t_pyo_tilde *)pd_new(pyo_tilde_class);

    x->chnls = (f) ? f : 2;
    x->bs = 64;
    x->sr = 44100.0;
    x->file = NULL;
    x->add = 0;
    x->debug = 0;

    /* create signal inlets (first is done in pyo_tilde_setup) */
    for (i=1; i<x->chnls; i++)
        inlet_new(&x->obj, &x->obj.ob_pd, &s_signal, &s_signal);
    /* create signal outlets */
    for (i=0; i<x->chnls; i++)
        outlet_new(&x->obj, &s_signal);

    x->in = (t_sample **)getbytes(x->chnls * sizeof(t_sample **));
    x->out = (t_sample **)getbytes(x->chnls * sizeof(t_sample **));
    x->msg = (char *)getbytes(262144 * sizeof(char *));

    for (i=0; i<x->chnls; i++)
        x->in[i] = x->out[i] = 0;

    x->interp = pyo_new_interpreter(x->sr, x->bs, x->chnls);
    
    x->inbuf = (float *)pyo_get_input_buffer_address(x->interp);
    x->outbuf = (float *)pyo_get_output_buffer_address(x->interp);
    x->callback = (void *)pyo_get_embedded_callback_address(x->interp);
    x->id = pyo_get_server_id(x->interp);

    return (void *)x;
}

void pyo_tilde_free(t_pyo_tilde *x) {
    freebytes(x->in, sizeof(x->in));
    freebytes(x->out, sizeof(x->out));
    freebytes(x->msg, sizeof(x->msg));
    pyo_end_interpreter(x->interp);
}

void pyo_tilde_set_value(t_pyo_tilde *x, char *att, int argc, t_atom *argv) {
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
        post("pyo~: %s", x->msg);
}

void pyo_tilde_value(t_pyo_tilde *x, t_symbol *s, int argc, t_atom *argv) {
    char *att = ".value";
    pyo_tilde_set_value(x, att, argc, argv);
}
void pyo_tilde_set(t_pyo_tilde *x, t_symbol *s, int argc, t_atom *argv) {
    char *att = "";
    pyo_tilde_set_value(x, att, argc, argv);
}

void pyo_tilde_create(t_pyo_tilde *x, t_symbol *s, int argc, t_atom *argv) {
    int err;
    const char *varname, *object;
    char fchar[32];
    t_symbol *c = atom_getsymbol(argv);
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
        post("pyo~: %s", x->msg);
}

void pyo_tilde_midi_event(t_pyo_tilde *x, t_symbol *s, int argc, t_atom *argv) {
    int status = 0, data1 = 0, data2 = 0;
    if (argc > 0)
        status = (int)atom_getfloat(argv);
    if (argc > 1)
        data1 = (int)atom_getfloat(++argv);
    if (argc > 2)
        data2 = (int)atom_getfloat(++argv);
    pyo_add_midi_event(x->interp, status, data1, data2);
}

void pyo_tilde_clear(t_pyo_tilde *x) {
    pyo_server_reboot(x->interp);
}

void pyo_tilde_debug(t_pyo_tilde *x, t_float debug) {
    x->debug = debug <= 0 ? 0 : 1;
}

void pyo_tilde_read(t_pyo_tilde *x, t_symbol *s, int argc, t_atom *argv) {
    int err;
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
    if (pyo_is_server_started(x->interp)) { 
        err = pyo_exec_file(x->interp, x->file, x->msg, x->add);
        if (err == 1) {
            post("Unable to open file < %s >", x->file);
            x->file = NULL;
        } else if (err == 2) {
            post("Bad code in file < %s >", x->file);
            x->file = NULL;
        }
    }
}

void pyo_tilde_call(t_pyo_tilde *x, t_symbol *s, int argc, t_atom *argv) {
    int err;
    char fchar[32];
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
        post("pyo~: %s", x->msg);
}

void pyo_tilde_setup(void) {
    pyo_tilde_class = class_new(gensym("pyo~"), (t_newmethod)pyo_tilde_new,
        (t_method)pyo_tilde_free, sizeof(t_pyo_tilde), CLASS_DEFAULT, A_DEFFLOAT, 0);
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
                    A_GIMME, 0); /* create a python object */
    class_addmethod(pyo_tilde_class, (t_method)pyo_tilde_debug, gensym("debug"), 
                    A_DEFFLOAT, 0); /* set the debug (verbose) mode */
    CLASS_MAINSIGNALIN(pyo_tilde_class, t_pyo_tilde, f);
}
