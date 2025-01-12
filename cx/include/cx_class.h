/* cx_class.h
 *
 * This file contains generated code. Do not modify!
 */

#ifndef cx_class_H
#define cx_class_H

#include "cortex.h"
#include "cx_struct.h"
#include "cx__type.h"
#include "cx__api.h"
#include "cx__meta.h"

#ifdef __cplusplus
extern "C" {
#endif

/* virtual ::cortex::lang::class::allocSize() */
cx_uint32 cx_class_allocSize(cx_class _this);

/* ::cortex::lang::class::allocSize() */
cx_uint32 cx_class_allocSize_v(cx_class _this);

/* ::cortex::lang::class::bindObserver(observer observer) */
cx_void cx_class_bindObserver(cx_class _this, cx_observer observer);

/* ::cortex::lang::class::construct() */
cx_int16 cx_class_construct(cx_class _this);

/* ::cortex::lang::class::destruct() */
cx_void cx_class_destruct(cx_class _this);

/* ::cortex::lang::class::findObserver(object observable,string expr) */
cx_observer cx_class_findObserver(cx_class _this, cx_object observable, cx_string expr);

/* ::cortex::lang::class::init() */
cx_int16 cx_class_init(cx_class _this);

/* ::cortex::lang::class::instanceof(object object) */
cx_bool cx_class_instanceof(cx_class _this, cx_object object);

/* ::cortex::lang::class::privateObserver(object object,observer observer) */
cx_observer cx_class_privateObserver(cx_class _this, cx_object object, cx_observer observer);

/* ::cortex::lang::class::resolveInterfaceMethod(interface interface,uint32 method) */
cx_method cx_class_resolveInterfaceMethod(cx_class _this, cx_interface interface, cx_uint32 method);

#ifdef __cplusplus
}
#endif
#endif

