/* cx_class.c
 *
 * This file contains the implementation for the generated interface.
 *
 * Don't mess with the begin and end tags, since these will ensure that modified
 * code in interface functions isn't replaced when code is re-generated.
 */

#include "cx.h"

/* $header() */
#include "cx__interface.h"

/* An object of a class-type has extra information appended to it's value. This is the information
* required to store callbacks and observers. The memory layout
* of a class-instantiation is (exclusive cortex object-header):
*
* +-----------------+
* |  object-value   |
* +-----------------+
* | callback-table  |
* +-----------------+
* | observer-table  |
* +-----------------+
*
*/

static cx_uint32 cx__class_observerCount(cx_class _this);

static cx_uint32 cx__class_observerCount(cx_class _this) {
    cx_uint32 count;
    cx_interface o;

    count = _this->observers.length;
    o = cx_interface(_this);
    while (o->base) {
        o = o->base;
        count += cx_class(o)->observers.length;
    }

    return count;
}

/* Check interfaces */
static cx_bool cx_class_checkInterfaceCompatibility(cx_class _this, cx_interface interface, cx_vtable* vtable) {
    cx_uint32 i;
    cx_method m_interface; /* Interface method */
    cx_method m_class, *m_classPtr; /* Class method */
    cx_bool compatible;
    cx_int32 distance;

    /* Walk vtable of interface */
    compatible = TRUE;
    for (i=0; (i<interface->methods.length); i++) {
        m_interface = (cx_method)interface->methods.buffer[i];

        m_class = NULL;
        m_classPtr = (cx_method*)cx_vtableLookup(&cx_interface(_this)->methods, cx_nameof(m_interface), NULL, &distance);
        if (m_classPtr) {
            m_class = *m_classPtr;
        }

        /* Check if procedures are compatible */
        if (m_class && !distance) {
            if ((compatible = cx_interface_checkProcedureCompatibility(cx_function(m_interface), cx_function(m_class)))) {
                cx_set_ext(_this, &vtable->buffer[i], m_class, "Set method in interface lookup table.");
            }
        } else {
            cx_id id, id2;
            cx_error("class '%s' is missing implementation for function '%s'", cx_fullname(_this, id), cx_fullname(m_interface, id2));
            compatible = FALSE;
        }
    }

    if (interface->base) {
        compatible = compatible & cx_class_checkInterfaceCompatibility(_this, interface->base, vtable);
    }

    return compatible;
}

/* Get table that stores observers */
cx_vtable* cx_class_getObserverVtable(cx_object o) {
    cx_vtable* result;
    cx_type type;
    cx_uint32 observerCount;

    type = cx_typeof(o);
    result = NULL;

    /* Obtain vtable. */
    if (cx_class_instanceof(cx_class_o, type)) {
        if ((observerCount = cx__class_observerCount(cx_class(type)))) {
            result = (cx_vtable*)CX_OFFSET(o, type->size);
        }
    }

    return result;
}

cx_object cx_class_getObservable(cx_class _this, cx_observer observer, cx_object me) {
    cx_vtable* observers;
    cx_object result = NULL;
    CX_UNUSED(_this);
    observers = cx_class_getObserverVtable(me);
    if (observers) {
        result = observers->buffer[observer->template-1];
    } else {
        cx_id id, id2;
        cx_error("failed to get observer for object '%s' of type '%s'", cx_fullname(me, id), cx_fullname(cx_typeof(me), id2));
    }
    return result;
}

void cx_class_setObservable(cx_class _this, cx_observer observer, cx_object me, cx_object observable) {
    cx_vtable* observers;
    CX_UNUSED(_this);

    observers = cx_class_getObserverVtable(me);
    if (observers) {
        observers->buffer[observer->template-1] = observable;
    } else {
        cx_id id, id2;
        cx_error("failed to set observer for object '%s' of type '%s'", cx_fullname(me, id), cx_fullname(cx_typeof(me), id2));
    }
}

void cx_class_attachObservers(cx_class _this, cx_object object) {
    cx_uint32 i, id;
    cx_vtable* observers;
    cx_class base;
    cx_object observable;

    /* Get table for instantiated observer-templates */
    observers = cx_class_getObserverVtable(object);
    if (observers) {
        id = cx__class_observerCount(_this);
        observers->buffer = CX_OFFSET(observers, sizeof(cx_vtable));
        observers->length = id;
        base = _this;

        do {
            for (i=0; i<base->observers.length; i++) {
                observable = base->observers.buffer[i]->observable;
                cx_class_setObservable(_this, base->observers.buffer[i], object, observable);
            }
        } while ((base = cx_class(cx_interface(base)->base)));
    }
}

void cx_class_listenObservers(cx_class _this, cx_object object) {
    cx_uint32 i;
    cx_vtable* observers;
    cx_class base;
    cx_object observable;

    /* Get table for instantiated observer-templates */
    observers = cx_class_getObserverVtable(object);
    if (observers) {
        base = _this;

        do {
            for (i=0; i<base->observers.length; i++) {
                observable = observers->buffer[i];
                if (!observable) {
                    observable = object;
                }
                if (!cx_listening(observable, base->observers.buffer[i], object)) {
                    /* Do not activate observers that listen for non-observables and childs on non-scoped objects */
                    if (cx_checkAttr(observable, CX_ATTR_OBSERVABLE) &&
                            (!(base->observers.buffer[i]->mask & CX_ON_SCOPE) || cx_checkAttr(object, CX_ATTR_SCOPED))) {
                        cx_listen(observable, base->observers.buffer[i], object);
                    }
                }
            }
        } while ((base = cx_class(cx_interface(base)->base)));
    }
}

void cx_class_detachObservers(cx_class _this, cx_object object) {
    cx_uint32 i, id;
    cx_vtable* observers;
    cx_class base;
    cx_object observable;

    /* Get table for instantiated observer-templates */
    observers = cx_class_getObserverVtable(object);
    if (observers) {
        id = cx__class_observerCount(_this);
        observers->buffer = CX_OFFSET(observers, sizeof(cx_vtable));
        observers->length = id;
        base = _this;
        do {
            for (i=0; i<base->observers.length; i++) {
                observable = cx_class_getObservable(base, base->observers.buffer[i], object);
                if (observable) {
                    /* Do not silence observers that listen for childs on non-scoped objects */
                    if (cx_checkAttr(observable, CX_ATTR_OBSERVABLE) &&
                            (!(base->observers.buffer[i]->mask & CX_ON_SCOPE) || cx_checkAttr(object, CX_ATTR_SCOPED))) {
                        cx_silence(observable, base->observers.buffer[i], object);
                    }
                }
            }
        } while ((base = cx_class(cx_interface(base)->base)));
    }
}

/* $end */

/* ::cortex::lang::class::allocSize() */
cx_uint32 cx_class_allocSize_v(cx_class _this) {
/* $begin(::cortex::lang::class::allocSize) */
    cx_uint32 observerCount;
    cx_uint32 size;

    size = cx_type(_this)->size;

    if ((observerCount = cx__class_observerCount(_this))) {
        size += sizeof(cx_vtable) + observerCount * sizeof(cx_observer);
    }

    return size;
/* $end */
}

/* ::cortex::lang::class::bindObserver(observer observer) */
cx_void cx_class_bindObserver(cx_class _this, cx_observer observer) {
/* $begin(::cortex::lang::class::bindObserver) */
    _this->observers.buffer = cx_realloc(_this->observers.buffer, (_this->observers.length + 1) * sizeof(cx_observer));
    _this->observers.buffer[_this->observers.length] = observer;
    _this->observers.length++;
    observer->template = _this->observers.length;
    cx_keep_ext(_this, observer, "Bind observer to class");
/* $end */
}

/* ::cortex::lang::class::construct() */
cx_int16 cx_class_construct(cx_class _this) {
/* $begin(::cortex::lang::class::construct) */
    cx_int16 result;
    cx_uint32 i;

    /* This will bind methods of potential base-class which is necessary before validating
     * whether this class correctly (fully) implements its interface. */
    result = cx_struct_construct(cx_struct(_this));

    /* Create interface vector */
    if (!result) {
        if (_this->implements.length) {
            _this->interfaceVector.length = _this->implements.length;
            _this->interfaceVector.buffer = cx_calloc(_this->implements.length * sizeof(cx_interfaceVector));
        }

        /* Validate that all interface interfaces are correctly implemented and generate interface tables. */
        for (i=0; (i<_this->implements.length) && !result; i++) {
            cx_interface interface = _this->implements.buffer[i];
            _this->interfaceVector.buffer[i].interface = interface;
            _this->interfaceVector.buffer[i].vector.length  = interface->methods.length;
            if (interface->methods.length) {
                _this->interfaceVector.buffer[i].vector.buffer = cx_calloc(interface->methods.length * sizeof(cx_function));
            } else {
                _this->interfaceVector.buffer[i].vector.buffer = NULL;
            }
            result = !cx_class_checkInterfaceCompatibility(_this, interface, &_this->interfaceVector.buffer[i].vector);
        }
    }

    return result;
/* $end */
}

/* ::cortex::lang::class::destruct() */
cx_void cx_class_destruct(cx_class _this) {
/* $begin(::cortex::lang::class::destruct) */
    cx_uint32 i,j;
    cx_interfaceVector *v;

    /* Free attached observers */
    for (i=0; i<_this->observers.length; i++) {
        cx_free_ext(_this, _this->observers.buffer[i], "Unbind observer from class");
    }
    cx_dealloc(_this->observers.buffer);
    _this->observers.buffer = NULL;
    _this->observers.length = 0;

    /* Free interfaceVector */
    for (i=0; i<_this->interfaceVector.length; i++) {
        v = &_this->interfaceVector.buffer[i];
        v->interface = NULL;
        for (j=0; j<v->vector.length; j++) {
            if (v->vector.buffer[j]) {
                cx_free_ext(_this, v->vector.buffer[j], "Unbind interface vector");
                v->vector.buffer[j] = NULL;
            }
        }
    }

    /* Call type::destruct */
    cx_interface_destruct(cx_interface(_this));
/* $end */
}

/* ::cortex::lang::class::findObserver(object observable,string expr) */
cx_observer cx_class_findObserver(cx_class _this, cx_object observable, cx_string expr) {
/* $begin(::cortex::lang::class::findObserver) */
    cx_uint32 i;
    cx_observer result = NULL;

    for (i=0; i<_this->observers.length; i++) {
        if (_this->observers.buffer[i]->observable == observable) {
            if (_this->observers.buffer[i]->expression && expr) {
                if (!strcmp(_this->observers.buffer[i]->expression, expr)) {
                    result = _this->observers.buffer[i];
                    break;
                }
            } else {
                result = _this->observers.buffer[i];
                break;
            }
        }
    }

    return result;
/* $end */
}

/* ::cortex::lang::class::init() */
cx_int16 cx_class_init(cx_class _this) {
/* $begin(::cortex::lang::class::init) */
    if (cx_struct_init(cx_struct(_this))) {
        goto error;
    }

    cx_type(_this)->reference = TRUE;
    cx_interface(_this)->kind = CX_CLASS;

    return 0;
error:
    return -1;
/* $end */
}

/* ::cortex::lang::class::instanceof(object object) */
cx_bool cx_class_instanceof(cx_class _this, cx_object object) {
/* $begin(::cortex::lang::class::instanceof) */
    cx_type t;
    cx_bool result;

    result = FALSE;
    t = cx_typeof(object);

    if (t->kind == CX_COMPOSITE) {
        if (cx_interface(t)->kind == CX_CLASS) {
            cx_interface p;
            p = (cx_interface)t;

            while (p && !result) {
                result = (p == (cx_interface)_this);
                p = p->base;
            }
        }
    }

    return result;
/* $end */
}

/* ::cortex::lang::class::privateObserver(object object,observer observer) */
cx_observer cx_class_privateObserver(cx_class _this, cx_object object, cx_observer observer) {
/* $begin(::cortex::lang::class::privateObserver) */
    CX_UNUSED(_this);
    CX_UNUSED(object);
    return observer; /* Workaround - this function can be removed */
/* $end */
}

/* ::cortex::lang::class::resolveInterfaceMethod(interface interface,uint32 method) */
cx_method cx_class_resolveInterfaceMethod(cx_class _this, cx_interface interface, cx_uint32 method) {
/* $begin(::cortex::lang::class::resolveInterfaceMethod) */
    cx_uint32 i;
    cx_interfaceVector *v;

    /* Lookup interface class */
    for (i=0; i<_this->interfaceVector.length; i++) {
        v = &_this->interfaceVector.buffer[i];
        if (v->interface == interface) {
            break;
        } else {
            v = NULL;
        }
    }

    if (!v) {
        cx_id id, id2;
        cx_error("class::resolveInterfaceMethod: class '%s' does not implement interface '%s'", cx_fullname(_this, id), cx_fullname(interface, id2));
        goto error;
    }

    return cx_method(v->vector.buffer[method-1]);
error:
    return NULL;
/* $end */
}
