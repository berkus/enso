/* -*- mode: C; c-basic-offset: 4 -*-
 *
 * Pycairo - Python bindings for cairo
 *
 * Copyright © 2003-2005 James Henstridge
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "pycairo-private.h"


/* Class Surface ---------------------------------------------------------- */

/* PycairoSurface_FromSurface
 * Create a new
 *   PycairoImageSurface,
 *   PycairoPDFSurface,
 *   PycairoPSSurface,
 *   PycairoSVGSurface,
 *   PycairoWin32Surface, or
 *   PycairoXlibSurface from a cairo_surface_t.
 * surface - a cairo_surface_t to 'wrap' into a Python object.
 *           it is unreferenced if the PycairoSurface creation fails, or if
 *           the cairo_surface_t has an error status
 * base - the base object used to create the context, or NULL.
 *        it is referenced to keep it alive while the cairo_surface_t is
 *        being used
 * Return value: New reference or NULL on failure
 */
PyObject *
PycairoSurface_FromSurface (cairo_surface_t *surface, PyObject *base)
{
    PyTypeObject *type = NULL;
    PyObject *o;

    assert (surface != NULL);

    if (Pycairo_Check_Status (cairo_surface_status (surface))) {
	cairo_surface_destroy (surface);
	return NULL;
    }

    switch (cairo_surface_get_type (surface)) {
    case CAIRO_SURFACE_TYPE_IMAGE:
	type = &PycairoImageSurface_Type;
	break;
#if CAIRO_HAS_PDF_SURFACE
    case CAIRO_SURFACE_TYPE_PDF:
	type = &PycairoPDFSurface_Type;
	break;
#endif
#if CAIRO_HAS_PS_SURFACE
    case CAIRO_SURFACE_TYPE_PS:
	type = &PycairoPSSurface_Type;
	break;
#endif
#if CAIRO_HAS_SVG_SURFACE
    case CAIRO_SURFACE_TYPE_SVG:
	type = &PycairoSVGSurface_Type;
	break;
#endif
#if CAIRO_HAS_WIN32_SURFACE
    case CAIRO_SURFACE_TYPE_WIN32:
	type = &PycairoWin32Surface_Type;
	break;
#endif
#if CAIRO_HAS_XLIB_SURFACE
    case CAIRO_SURFACE_TYPE_XLIB:
	type = &PycairoXlibSurface_Type;
	break;
#endif
#if CAIRO_HAS_QUARTZ_SURFACE
    case CAIRO_SURFACE_TYPE_QUARTZ:
        type = &PycairoQuartzSurface_Type;
        break;
#endif
    default:
	PyErr_SetString(CairoError, "Unsupported Surface type");
	return NULL;
    }

    o = type->tp_alloc (type, 0);
    if (o == NULL) {
	cairo_surface_destroy (surface);
    } else {
	((PycairoSurface *)o)->surface = surface;
	Py_XINCREF(base);
	((PycairoSurface *)o)->base = base;
    }
    return o;
}

/* for use with
 * cairo_surface_write_to_png_stream()
 * cairo_pdf/ps/svg_surface_create_for_stream()
 */
static cairo_status_t
_write_func (void *closure, const unsigned char *data, unsigned int length)
{
    PyObject *res = PyObject_CallMethod ((PyObject *)closure, "write", "(s#)",
					 data, (Py_ssize_t)length);
    if (res == NULL) {
	/* an exception has occurred, it will be picked up later by
	 * Pycairo_Check_Status()
	 */
	return CAIRO_STATUS_WRITE_ERROR;
    }
    Py_DECREF(res);
    return CAIRO_STATUS_SUCCESS;
}

static void
surface_dealloc (PycairoSurface *o)
{
    if (o->surface) {
	cairo_surface_destroy(o->surface);
	o->surface = NULL;
    }
    Py_CLEAR(o->base);

    o->ob_type->tp_free((PyObject *)o);
}

static PyObject *
surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_TypeError,
		    "The Surface type cannot be instantiated");
    return NULL;
}

static PyObject *
surface_create_similar (PycairoSurface *o, PyObject *args)
{
    cairo_content_t content;
    int width, height;

    if (!PyArg_ParseTuple (args, "iii:Surface.create_similar",
			   &content, &width, &height))
	return NULL;
    return PycairoSurface_FromSurface (
	     cairo_surface_create_similar (o->surface, content, width, height),
	     NULL);
}

static PyObject *
surface_finish (PycairoSurface *o)
{
    cairo_surface_finish (o->surface);
    Py_CLEAR(o->base);
    RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
    Py_RETURN_NONE;
}

static PyObject *
surface_flush (PycairoSurface *o)
{
    cairo_surface_flush (o->surface);
    RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
    Py_RETURN_NONE;
}

static PyObject *
surface_get_content (PycairoSurface *o)
{
    return PyInt_FromLong (cairo_surface_get_content (o->surface));
}

static PyObject *
surface_get_device_offset (PycairoSurface *o)
{
    double x_offset, y_offset;
    cairo_surface_get_device_offset (o->surface, &x_offset, &y_offset);
    return Py_BuildValue("(dd)", x_offset, y_offset);
}

static PyObject *
surface_get_font_options (PycairoSurface *o)
{
    cairo_font_options_t *options = cairo_font_options_create();

    cairo_surface_get_font_options (o->surface, options);
    /* there is no reference fn */
    return PycairoFontOptions_FromFontOptions (options);
}

static PyObject *
surface_mark_dirty (PycairoSurface *o, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"x", "y", "width", "height", NULL};
    int x = 0, y = 0, width = -1, height = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwds,
				     "|iiii:Surface.mark_dirty", kwlist,
				     &x, &y, &width, &height))
	return NULL;

    cairo_surface_mark_dirty_rectangle (o->surface, x, y, width, height);
    RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
    Py_RETURN_NONE;
}

static PyObject *
surface_set_device_offset (PycairoSurface *o, PyObject *args)
{
    double x_offset, y_offset;

    if (!PyArg_ParseTuple (args, "dd:Surface.set_device_offset",
			   &x_offset, &y_offset))
	return NULL;

    cairo_surface_set_device_offset (o->surface, x_offset, y_offset);
    Py_RETURN_NONE;
}

static PyObject *
surface_set_fallback_resolution (PycairoSurface *o, PyObject *args)
{
    double x_ppi, y_ppi;

    if (!PyArg_ParseTuple(args, "dd:Surface.set_fallback_resolution",
			  &x_ppi, &y_ppi))
	return NULL;
    cairo_surface_set_fallback_resolution (o->surface, x_ppi, y_ppi);
    Py_RETURN_NONE;
}

#ifdef CAIRO_HAS_PNG_FUNCTIONS
/* METH_O */
static PyObject *
surface_write_to_png (PycairoSurface *o, PyObject *file)
{
    cairo_status_t status;

    if (PyObject_TypeCheck (file, &PyBaseString_Type)) {
	/* string (filename) argument */
	status = cairo_surface_write_to_png (o->surface,
					     PyString_AsString(file));

    } else {  /* file or file-like object argument */
	PyObject* writer = PyObject_GetAttrString (file, "write");
	if (writer == NULL || !PyCallable_Check (writer)) {
	    Py_XDECREF(writer);
	    PyErr_SetString(PyExc_TypeError,
"Surface.write_to_png takes one argument which must be a filename (str), file "
"object, or a file-like object which has a \"write\" method (like StringIO)");
	    return NULL;
	}
	Py_DECREF(writer);
	status = cairo_surface_write_to_png_stream (o->surface, _write_func,
						    file);
    }
    RETURN_NULL_IF_CAIRO_ERROR(status);
    Py_RETURN_NONE;
}
#endif  /* CAIRO_HAS_PNG_FUNCTIONS */


static PyMethodDef surface_methods[] = {
    /* methods never exposed in a language binding:
     * cairo_surface_destroy()
     * cairo_surface_get_type()
     * cairo_surface_get_user_data()
     * cairo_surface_reference()
     * cairo_surface_set_user_data()
     */
    {"create_similar", (PyCFunction)surface_create_similar,    METH_VARARGS },
    {"finish",         (PyCFunction)surface_finish,            METH_NOARGS },
    {"flush",          (PyCFunction)surface_flush,             METH_NOARGS },
    {"get_content",    (PyCFunction)surface_get_content,       METH_NOARGS },
    {"get_device_offset",(PyCFunction)surface_get_device_offset,
                                                                METH_NOARGS },
    {"get_font_options",(PyCFunction)surface_get_font_options, METH_NOARGS },
    {"mark_dirty",     (PyCFunction)surface_mark_dirty,        METH_KEYWORDS },
    {"set_device_offset",(PyCFunction)surface_set_device_offset,
                                                                METH_VARARGS },
    {"set_fallback_resolution",(PyCFunction)surface_set_fallback_resolution,
                                                                METH_VARARGS },
#ifdef CAIRO_HAS_PNG_FUNCTIONS
    {"write_to_png",   (PyCFunction)surface_write_to_png,      METH_O },
#endif
    {NULL, NULL, 0, NULL},
};


PyTypeObject PycairoSurface_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "enso.platform.osx.cairo.Surface",                    /* tp_name */
    sizeof(PycairoSurface),             /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)surface_dealloc,        /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,/* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    surface_methods,                    /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0, /* &PyBaseObject_Type, */        /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    (newfunc)surface_new,               /* tp_new */
    0,                                  /* tp_free */
    0,                                  /* tp_is_gc */
    0,                                  /* tp_bases */
};


/* Class ImageSurface(Surface) -------------------------------------------- */

static PyObject *
image_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    cairo_format_t format;
    int width, height;

    if (!PyArg_ParseTuple (args, "iii:ImageSurface.__new__",
			   &format, &width, &height))
	return NULL;
    return PycairoSurface_FromSurface (
	       cairo_image_surface_create (format, width, height),
	       NULL);
}

static PyObject *
image_surface_create_for_data (PyTypeObject *type, PyObject *args)
{
    cairo_surface_t *surface;
    cairo_format_t format;
    unsigned char *buffer;
    int width, height, stride = -1, res;
    Py_ssize_t buffer_len;
    PyObject *obj;

    if (!PyArg_ParseTuple(args, "Oiii|i:Surface.create_for_data",
			  &obj, &format, &width, &height, &stride))
	return NULL;

    res = PyObject_AsWriteBuffer (obj, (void **)&buffer, &buffer_len);
    if (res == -1)
	return NULL;

    if (width <= 0) {
	PyErr_SetString(PyExc_ValueError, "width must be positive");
	return NULL;
    }
    if (height <= 0) {
	PyErr_SetString(PyExc_ValueError, "height must be positive");
	return NULL;
    }
    /* if stride is missing, calculate it from width */
    if (stride < 0) {
	switch (format) {
	case CAIRO_FORMAT_ARGB32:
	case CAIRO_FORMAT_RGB24:
	    stride = width * 4;
	    break;
	case CAIRO_FORMAT_RGB16_565:
	    stride = width * 2;
	    break;
	case CAIRO_FORMAT_A8:
	    stride = width;
	    break;
	case CAIRO_FORMAT_A1: /* should be stride = (width + 31) / 32 * 4 ? */
	    stride = (width + 1) / 8;
	    break;
	default:
	    PyErr_SetString(CairoError, "Unknown format");
	    return NULL;
	}
    }
    if (height * stride > buffer_len) {
	PyErr_SetString(PyExc_TypeError, "buffer is not long enough");
	return NULL;
    }
    surface = cairo_image_surface_create_for_data (buffer, format, width,
						   height, stride);
    return PycairoSurface_FromSurface(surface, obj);
}


#ifdef CAIRO_HAS_PNG_FUNCTIONS
static cairo_status_t
_read_func (void *closure, unsigned char *data, unsigned int length)
{
    char *str;
    PyObject *pystr = PyObject_CallMethod ((PyObject *)closure, "read", "(i)",
					   length);
    if (pystr == NULL)
	return CAIRO_STATUS_READ_ERROR;
    str = PyString_AsString(pystr);
    Py_DECREF(pystr);
    if (str == NULL)
	return CAIRO_STATUS_READ_ERROR;
    /* don't use strncpy() since png data may contain NUL bytes */
    memcpy (data, str, length);
    return CAIRO_STATUS_SUCCESS;
}

/* METH_O | METH_CLASS */
static PyObject *
image_surface_create_from_png (PyTypeObject *type, PyObject *file)
{
    PyObject* reader;

    if (PyObject_TypeCheck (file, &PyBaseString_Type)) {
	return PycairoSurface_FromSurface (
            cairo_image_surface_create_from_png (PyString_AsString(file)),
            NULL);
    }

    /* file or file-like object argument */
    reader = PyObject_GetAttrString (file, "read");
    if (reader == NULL || !PyCallable_Check (reader)) {
	Py_XDECREF(reader);
	PyErr_SetString(PyExc_TypeError,
"ImageSurface.create_from_png argument must be a filename (str), file object, "
"or an object that has a \"read\" method (like StringIO)");
	return NULL;
    }
    Py_DECREF(reader);

    return PycairoSurface_FromSurface (
        cairo_image_surface_create_from_png_stream (_read_func, file), NULL);
}
#endif /* CAIRO_HAS_PNG_FUNCTIONS */

static PyObject *
image_surface_get_data (PycairoImageSurface *o)
{
    return PyBuffer_FromReadWriteObject((PyObject *)o, 0, Py_END_OF_BUFFER);
}

static PyObject *
image_surface_get_format (PycairoImageSurface *o)
{
    return PyInt_FromLong (cairo_image_surface_get_format (o->surface));
}

static PyObject *
image_surface_get_height (PycairoImageSurface *o)
{
    return PyInt_FromLong (cairo_image_surface_get_height (o->surface));
}

static PyObject *
image_surface_get_width (PycairoImageSurface *o)
{
    return PyInt_FromLong (cairo_image_surface_get_width (o->surface));
}

static PyObject *
image_surface_get_stride (PycairoImageSurface *o)
{
    return PyInt_FromLong (cairo_image_surface_get_stride (o->surface));
}


/* Buffer interface functions, used by ImageSurface.get_data() */
static int
image_surface_buffer_getreadbuf (PycairoImageSurface *o, int segment,
				 const void **ptr)
{
    cairo_surface_t *surface = o->surface;
    int height, stride;

    if (segment != 0) {
	PyErr_SetString(PyExc_SystemError,
			"accessing non-existent ImageSurface segment");
	return -1;
    }
    height = cairo_image_surface_get_height (surface);
    stride = cairo_image_surface_get_stride (surface);
    *ptr = (void *) cairo_image_surface_get_data (surface);
    return height * stride;
}

static int
image_surface_buffer_getwritebuf (PycairoImageSurface *o, int segment,
				  const void **ptr)
{
    cairo_surface_t *surface = o->surface;
    int height, stride;

    if (segment != 0) {
	PyErr_SetString(PyExc_SystemError,
			"accessing non-existent ImageSurface segment");
	return -1;
    }
    height = cairo_image_surface_get_height (surface);
    stride = cairo_image_surface_get_stride (surface);
    *ptr = (void *) cairo_image_surface_get_data (surface);
    return height * stride;
}

static int
image_surface_buffer_getsegcount (PycairoImageSurface *o, int *lenp)
{
    if (lenp) {
	/* report the sum of the sizes (in bytes) of all segments */
	cairo_surface_t *surface = o->surface;
	int height = cairo_image_surface_get_height (surface);
	int stride = cairo_image_surface_get_stride (surface);
	*lenp = height * stride;
    }
    return 1;  /* surface data is all in one segment */
}

/* See Python C API Manual 10.7 */
static PyBufferProcs image_surface_as_buffer = {
    (readbufferproc) image_surface_buffer_getreadbuf,
    (writebufferproc)image_surface_buffer_getwritebuf,
    (segcountproc)   image_surface_buffer_getsegcount,
    (charbufferproc) NULL,
};

static PyMethodDef image_surface_methods[] = {
    {"create_for_data",(PyCFunction)image_surface_create_for_data,
                                                   METH_VARARGS | METH_CLASS },
#ifdef CAIRO_HAS_PNG_FUNCTIONS
    {"create_from_png", (PyCFunction)image_surface_create_from_png,
                                                   METH_O | METH_CLASS },
#endif
    {"get_data",      (PyCFunction)image_surface_get_data,       METH_NOARGS},
    {"get_format",    (PyCFunction)image_surface_get_format,     METH_NOARGS},
    {"get_height",    (PyCFunction)image_surface_get_height,     METH_NOARGS},
    {"get_width",     (PyCFunction)image_surface_get_width,      METH_NOARGS},
    {"get_stride",    (PyCFunction)image_surface_get_stride,     METH_NOARGS},
    {NULL, NULL, 0, NULL},
};


PyTypeObject PycairoImageSurface_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "enso.platform.osx.cairo.ImageSurface",               /* tp_name */
    sizeof(PycairoImageSurface),        /* tp_basicsize */
    0,                                  /* tp_itemsize */
    0,                                  /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    &image_surface_as_buffer,		/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    image_surface_methods,              /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0, /* &PycairoSurface_Type, */      /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    (newfunc)image_surface_new,         /* tp_new */
    0,                                  /* tp_free */
    0,                                  /* tp_is_gc */
    0,                                  /* tp_bases */
};


/* Class PDFSurface(Surface) ---------------------------------------------- */
#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>

static PyObject *
pdf_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    double width_in_points, height_in_points;
    PyObject *file, *writer;

    if (!PyArg_ParseTuple(args, "Odd:PDFSurface.__new__",
			  &file, &width_in_points, &height_in_points))
	return NULL;

    if (PyObject_TypeCheck (file, &PyBaseString_Type)) {
	/* string (filename) argument */
	return PycairoSurface_FromSurface (
                  cairo_pdf_surface_create (PyString_AsString(file),
                                     width_in_points, height_in_points),
	       NULL);
    }
    /* file or file-like object argument */
    writer = PyObject_GetAttrString (file, "write");
    if (writer == NULL || !PyCallable_Check (writer)) {
	Py_XDECREF(writer);
	PyErr_SetString(PyExc_TypeError,
"PDFSurface argument 1 must be a filename (str), file object, or an object "
"that has a \"write\" method (like StringIO)");
	return NULL;
    }
    Py_DECREF(writer);

    return PycairoSurface_FromSurface (
	       cairo_pdf_surface_create_for_stream (_write_func, file,
		   width_in_points, height_in_points),
	   file);
}

static PyObject *
pdf_surface_set_size (PycairoPDFSurface *o, PyObject *args)
{
    double width_in_points, height_in_points;

    if (!PyArg_ParseTuple(args, "dd:PDFSurface.set_size", &width_in_points,
			  &height_in_points))
	return NULL;
    cairo_pdf_surface_set_size (o->surface, width_in_points,
				height_in_points);
    Py_RETURN_NONE;
}

static PyMethodDef pdf_surface_methods[] = {
    {"set_size", (PyCFunction)pdf_surface_set_size,    METH_VARARGS },
    {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoPDFSurface_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "enso.platform.osx.cairo.PDFSurface",                 /* tp_name */
    sizeof(PycairoPDFSurface),          /* tp_basicsize */
    0,                                  /* tp_itemsize */
    0,                                  /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    pdf_surface_methods,                /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0, /* &PycairoSurface_Type, */      /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    (newfunc)pdf_surface_new,           /* tp_new */
    0,                                  /* tp_free */
    0,                                  /* tp_is_gc */
    0,                                  /* tp_bases */
};
#endif /* CAIRO_HAS_PDF_SURFACE */


/* Class PSSurface(Surface) ----------------------------------------------- */
#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>

static PyObject *
ps_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    double width_in_points, height_in_points;
    PyObject *file, *writer;

    if (!PyArg_ParseTuple(args, "Odd:PSSurface.__new__",
			  &file, &width_in_points, &height_in_points))
	return NULL;

    if (PyObject_TypeCheck (file, &PyBaseString_Type)) {
	/* string (filename) argument */
	return PycairoSurface_FromSurface (
                  cairo_ps_surface_create (PyString_AsString(file),
                                     width_in_points, height_in_points),
	       NULL);

    }
    /* else: file or file-like object argument */
    writer = PyObject_GetAttrString (file, "write");
    if (writer == NULL || !PyCallable_Check (writer)) {
	Py_XDECREF(writer);
	PyErr_SetString(PyExc_TypeError,
"PSSurface argument 1 must be a filename (str), file object, or an object "
"that has a \"write\" method (like StringIO)");
	return NULL;
    }
    Py_DECREF(writer);

    return PycairoSurface_FromSurface (
	       cairo_ps_surface_create_for_stream (_write_func, file,
                   width_in_points, height_in_points),
	       file);
}

static PyObject *
ps_surface_dsc_begin_page_setup (PycairoPSSurface *o)
{
    cairo_ps_surface_dsc_begin_page_setup (o->surface);
    RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
    Py_RETURN_NONE;
}

static PyObject *
ps_surface_dsc_begin_setup (PycairoPSSurface *o)
{
    cairo_ps_surface_dsc_begin_setup (o->surface);
    RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
    Py_RETURN_NONE;
}

static PyObject *
ps_surface_dsc_comment (PycairoPSSurface *o, PyObject *args)
{
    const char *comment;
    if (!PyArg_ParseTuple(args, "s:PSSurface.dsc_comment", &comment))
	return NULL;
    cairo_ps_surface_dsc_comment (o->surface, comment);
    RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
    Py_RETURN_NONE;
}

static PyObject *
ps_surface_set_size (PycairoPSSurface *o, PyObject *args)
{
    double width_in_points, height_in_points;

    if (!PyArg_ParseTuple(args, "dd:PSSurface.set_size",
			  &width_in_points, &height_in_points))
	return NULL;
    cairo_ps_surface_set_size (o->surface, width_in_points, height_in_points);
    Py_RETURN_NONE;
}

static PyMethodDef ps_surface_methods[] = {
    {"dsc_begin_page_setup",
                   (PyCFunction)ps_surface_dsc_begin_page_setup, METH_NOARGS },
    {"dsc_begin_setup", (PyCFunction)ps_surface_dsc_begin_setup, METH_NOARGS },
    {"dsc_comment", (PyCFunction)ps_surface_dsc_comment,        METH_VARARGS },
    {"set_size", (PyCFunction)ps_surface_set_size,              METH_VARARGS },
    {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoPSSurface_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "enso.platform.osx.cairo.PSSurface",                  /* tp_name */
    sizeof(PycairoPSSurface),           /* tp_basicsize */
    0,                                  /* tp_itemsize */
    0,                                  /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    ps_surface_methods,                 /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0, /* &PycairoSurface_Type, */      /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    (newfunc)ps_surface_new,            /* tp_new */
    0,                                  /* tp_free */
    0,                                  /* tp_is_gc */
    0,                                  /* tp_bases */
};
#endif  /* CAIRO_HAS_PS_SURFACE */


/* Class SVGSurface(Surface) ----------------------------------------------- */
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>

static PyObject *
svg_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    double width_in_points, height_in_points;
    PyObject *file, *writer;

    if (!PyArg_ParseTuple(args, "Odd:SVGSurface.__new__",
			  &file, &width_in_points, &height_in_points))
	return NULL;

    if (PyObject_TypeCheck (file, &PyBaseString_Type)) {
	/* string (filename) argument */
	return PycairoSurface_FromSurface (
                  cairo_svg_surface_create (PyString_AsString(file),
                                     width_in_points, height_in_points),
	       NULL);
    }
    /* else: file or file-like object argument */
    writer = PyObject_GetAttrString (file, "write");
    if (writer == NULL || !PyCallable_Check (writer)) {
	Py_XDECREF(writer);
	PyErr_SetString(PyExc_TypeError,
"SVGSurface argument 1 must be a filename (str), file object, or an object "
"that has a \"write\" method (like StringIO)");
	return NULL;
    }
    Py_DECREF(writer);

    return PycairoSurface_FromSurface (
	       cairo_svg_surface_create_for_stream (_write_func, file,
	           width_in_points, height_in_points),
	       file);
}

static PyMethodDef svg_surface_methods[] = {
    /* TODO
     * cairo_svg_surface_restrict_to_version
     * cairo_svg_get_versions
     * cairo_svg_version_to_string
     */
    {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoSVGSurface_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "enso.platform.osx.cairo.SVGSurface",                 /* tp_name */
    sizeof(PycairoSVGSurface),          /* tp_basicsize */
    0,                                  /* tp_itemsize */
    0,                                  /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    svg_surface_methods,                /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0, /* &PycairoSurface_Type, */      /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    (newfunc)svg_surface_new,           /* tp_new */
    0,                                  /* tp_free */
    0,                                  /* tp_is_gc */
    0,                                  /* tp_bases */
};
#endif  /* CAIRO_HAS_SVG_SURFACE */


/* Class Win32Surface(Surface) -------------------------------------------- */
#if CAIRO_HAS_WIN32_SURFACE
#include <cairo-win32.h>

static PyObject *
win32_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int hdc;

    if (!PyArg_ParseTuple(args, "i:Win32Surface.__new__", &hdc))
	return NULL;
    return PycairoSurface_FromSurface (
	       cairo_win32_surface_create ((HDC)hdc), NULL);
}

static PyMethodDef win32_surface_methods[] = {
    {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoWin32Surface_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "enso.platform.osx.cairo.Win32Surface",               /* tp_name */
    sizeof(PycairoWin32Surface),        /* tp_basicsize */
    0,                                  /* tp_itemsize */
    0,                                  /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    win32_surface_methods,              /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0, /* &PycairoSurface_Type, */      /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    (newfunc)win32_surface_new,         /* tp_new */
    0,                                  /* tp_free */
    0,                                  /* tp_is_gc */
    0,                                  /* tp_bases */
};
#endif  /* CAIRO_HAS_WIN32_SURFACE */


/* Class XlibSurface(Surface) --------------------------------------------- */
#ifdef CAIRO_HAS_XLIB_SURFACE
#include <cairo-xlib.h>

static PyObject *
xlib_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_TypeError,
		    "The XlibSurface type cannot be directly instantiated");
    return NULL;
}

static PyObject *
xlib_surface_get_depth (PycairoXlibSurface *o)
{
    return PyInt_FromLong (cairo_xlib_surface_get_depth (o->surface));
}

static PyObject *
xlib_surface_get_height (PycairoXlibSurface *o)
{
    return PyInt_FromLong (cairo_xlib_surface_get_height (o->surface));
}

static PyObject *
xlib_surface_get_width (PycairoXlibSurface *o)
{
    return PyInt_FromLong (cairo_xlib_surface_get_width (o->surface));
}

static PyMethodDef xlib_surface_methods[] = {
    {"get_depth", (PyCFunction)xlib_surface_get_depth,    METH_NOARGS },
    {"get_height",(PyCFunction)xlib_surface_get_height,   METH_NOARGS },
    {"get_width", (PyCFunction)xlib_surface_get_width,    METH_NOARGS },
    {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoXlibSurface_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "enso.platform.osx.cairo.XlibSurface",                /* tp_name */
    sizeof(PycairoXlibSurface),         /* tp_basicsize */
    0,                                  /* tp_itemsize */
    0,                                  /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    xlib_surface_methods,               /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0, /* &PycairoSurface_Type, */      /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    (newfunc)xlib_surface_new,          /* tp_new */
    0,                                  /* tp_free */
    0,                                  /* tp_is_gc */
    0,                                  /* tp_bases */
};
#endif  /* CAIRO_HAS_XLIB_SURFACE */

/* Class QuartzSurface(Surface) -------------------------------------------- */
#if CAIRO_HAS_QUARTZ_SURFACE
#include <cairo-quartz.h>

static PyObject *
quartz_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_TypeError,
		    "The QuartzSurface type cannot be directly instantiated");
    return NULL;
}

static PyMethodDef quartz_surface_methods[] = {
    {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoQuartzSurface_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "enso.platform.osx.cairo.QuartzSurface",               /* tp_name */
    sizeof(PycairoQuartzSurface),        /* tp_basicsize */
    0,                                  /* tp_itemsize */
    0,                                  /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    quartz_surface_methods,              /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0, /* &PycairoSurface_Type, */      /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    (newfunc)quartz_surface_new,         /* tp_new */
    0,                                  /* tp_free */
    0,                                  /* tp_is_gc */
    0,                                  /* tp_bases */
};
#endif  /* CAIRO_HAS_QUARTZ_SURFACE */
