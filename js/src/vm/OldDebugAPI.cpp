/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * JS debugging API.
 */

#include "js/OldDebugAPI.h"

#include <string.h>

#include "jscntxt.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsprf.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jstypes.h"
#include "jswatchpoint.h"

#include "frontend/SourceNotes.h"
#include "jit/AsmJSModule.h"
#include "vm/Debugger.h"
#include "vm/Shape.h"

#include "jsatominlines.h"
#include "jsinferinlines.h"
#include "jsscriptinlines.h"

#include "vm/Debugger-inl.h"
#include "vm/Interpreter-inl.h"
#include "vm/Stack-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::PodZero;

JS_PUBLIC_API(bool)
JS_GetDebugMode(JSContext *cx)
{
    return cx->compartment()->debugMode();
}

JS_PUBLIC_API(bool)
JS_SetDebugMode(JSContext *cx, bool debug)
{
    return JS_SetDebugModeForCompartment(cx, cx->compartment(), debug);
}

JS_PUBLIC_API(void)
JS_SetRuntimeDebugMode(JSRuntime *rt, bool debug)
{
    rt->debugMode = !!debug;
}

JSTrapStatus
js::ScriptDebugPrologue(JSContext *cx, AbstractFramePtr frame, jsbytecode *pc)
{
    JS_ASSERT_IF(frame.isInterpreterFrame(), frame.asInterpreterFrame() == cx->interpreterFrame());

    RootedValue rval(cx);
    JSTrapStatus status = Debugger::onEnterFrame(cx, frame, &rval);
    switch (status) {
      case JSTRAP_CONTINUE:
        break;
      case JSTRAP_THROW:
        cx->setPendingException(rval);
        break;
      case JSTRAP_ERROR:
        cx->clearPendingException();
        break;
      case JSTRAP_RETURN:
        frame.setReturnValue(rval);
        break;
      default:
        MOZ_CRASH("bad Debugger::onEnterFrame JSTrapStatus value");
    }
    return status;
}

bool
js::ScriptDebugEpilogue(JSContext *cx, AbstractFramePtr frame, jsbytecode *pc, bool okArg)
{
    JS_ASSERT_IF(frame.isInterpreterFrame(), frame.asInterpreterFrame() == cx->interpreterFrame());

    bool ok = okArg;

    return Debugger::onLeaveFrame(cx, frame, ok);
}

JSTrapStatus
js::DebugExceptionUnwind(JSContext *cx, AbstractFramePtr frame, jsbytecode *pc)
{
    JS_ASSERT(cx->compartment()->debugMode());

    if (cx->compartment()->getDebuggees().empty())
        return JSTRAP_CONTINUE;

    /* Call debugger throw hook if set. */
    RootedValue rval(cx);
    JSTrapStatus status = Debugger::onExceptionUnwind(cx, &rval);

    switch (status) {
      case JSTRAP_ERROR:
        cx->clearPendingException();
        break;

      case JSTRAP_RETURN:
        cx->clearPendingException();
        frame.setReturnValue(rval);
        break;

      case JSTRAP_THROW:
        cx->setPendingException(rval);
        break;

      case JSTRAP_CONTINUE:
        break;

      default:
        MOZ_CRASH("Invalid trap status");
    }

    return status;
}

JS_FRIEND_API(bool)
JS_SetDebugModeForAllCompartments(JSContext *cx, bool debug)
{
    for (ZonesIter zone(cx->runtime(), SkipAtoms); !zone.done(); zone.next()) {
        // Invalidate a zone at a time to avoid doing a ZoneCellIter
        // per compartment.
        AutoDebugModeInvalidation invalidate(zone);
        for (CompartmentsInZoneIter c(zone); !c.done(); c.next()) {
            // Ignore special compartments (atoms, JSD compartments)
            if (c->principals) {
                if (!c->setDebugModeFromC(cx, !!debug, invalidate))
                    return false;
            }
        }
    }
    return true;
}

JS_FRIEND_API(bool)
JS_SetDebugModeForCompartment(JSContext *cx, JSCompartment *comp, bool debug)
{
    AutoDebugModeInvalidation invalidate(comp);
    return comp->setDebugModeFromC(cx, !!debug, invalidate);
}

static bool
CheckDebugMode(JSContext *cx)
{
    bool debugMode = JS_GetDebugMode(cx);
    /*
     * :TODO:
     * This probably should be an assertion, since it's indicative of a severe
     * API misuse.
     */
    if (!debugMode) {
        JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage,
                                     nullptr, JSMSG_NEED_DEBUG_MODE);
    }
    return debugMode;
}

JS_PUBLIC_API(bool)
JS_SetSingleStepMode(JSContext *cx, HandleScript script, bool singleStep)
{
    assertSameCompartment(cx, script);

    if (!CheckDebugMode(cx))
        return false;

    return script->setStepModeFlag(cx, singleStep);
}

/************************************************************************/

JS_PUBLIC_API(bool)
JS_SetWatchPoint(JSContext *cx, HandleObject origobj, HandleId id,
                 JSWatchPointHandler handler, HandleObject closure)
{
    assertSameCompartment(cx, origobj);

    RootedObject obj(cx, GetInnerObject(origobj));
    if (!obj)
        return false;

    if (!obj->isNative() || obj->is<TypedArrayObject>()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_WATCH,
                             obj->getClass()->name);
        return false;
    }

    /*
     * Use sparse indexes for watched objects, as dense elements can be written
     * to without checking the watchpoint map.
     */
    if (!JSObject::sparsifyDenseElements(cx, obj))
        return false;

    types::MarkTypePropertyNonData(cx, obj, id);

    WatchpointMap *wpmap = cx->compartment()->watchpointMap;
    if (!wpmap) {
        wpmap = cx->runtime()->new_<WatchpointMap>();
        if (!wpmap || !wpmap->init()) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        cx->compartment()->watchpointMap = wpmap;
    }
    return wpmap->watch(cx, obj, id, handler, closure);
}

JS_PUBLIC_API(bool)
JS_ClearWatchPoint(JSContext *cx, JSObject *obj, jsid id,
                   JSWatchPointHandler *handlerp, JSObject **closurep)
{
    assertSameCompartment(cx, obj, id);

    if (WatchpointMap *wpmap = cx->compartment()->watchpointMap)
        wpmap->unwatch(obj, id, handlerp, closurep);
    return true;
}

JS_PUBLIC_API(bool)
JS_ClearWatchPointsForObject(JSContext *cx, JSObject *obj)
{
    assertSameCompartment(cx, obj);

    if (WatchpointMap *wpmap = cx->compartment()->watchpointMap)
        wpmap->unwatchObject(obj);
    return true;
}

/************************************************************************/

JS_PUBLIC_API(unsigned)
JS_PCToLineNumber(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    return js::PCToLineNumber(script, pc);
}

JS_PUBLIC_API(jsbytecode *)
JS_LineNumberToPC(JSContext *cx, JSScript *script, unsigned lineno)
{
    return js_LineNumberToPC(script, lineno);
}

JS_PUBLIC_API(jsbytecode *)
JS_EndPC(JSContext *cx, JSScript *script)
{
    return script->codeEnd();
}

JS_PUBLIC_API(bool)
JS_GetLinePCs(JSContext *cx, JSScript *script,
              unsigned startLine, unsigned maxLines,
              unsigned* count, unsigned** retLines, jsbytecode*** retPCs)
{
    size_t len = (script->length() > maxLines ? maxLines : script->length());
    unsigned *lines = cx->pod_malloc<unsigned>(len);
    if (!lines)
        return false;

    jsbytecode **pcs = cx->pod_malloc<jsbytecode*>(len);
    if (!pcs) {
        js_free(lines);
        return false;
    }

    unsigned lineno = script->lineno();
    unsigned offset = 0;
    unsigned i = 0;
    for (jssrcnote *sn = script->notes(); !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn)) {
        offset += SN_DELTA(sn);
        SrcNoteType type = (SrcNoteType) SN_TYPE(sn);
        if (type == SRC_SETLINE || type == SRC_NEWLINE) {
            if (type == SRC_SETLINE)
                lineno = (unsigned) js_GetSrcNoteOffset(sn, 0);
            else
                lineno++;

            if (lineno >= startLine) {
                lines[i] = lineno;
                pcs[i] = script->offsetToPC(offset);
                if (++i >= maxLines)
                    break;
            }
        }
    }

    *count = i;
    if (retLines)
        *retLines = lines;
    else
        js_free(lines);

    if (retPCs)
        *retPCs = pcs;
    else
        js_free(pcs);

    return true;
}

JS_PUBLIC_API(unsigned)
JS_GetFunctionArgumentCount(JSContext *cx, JSFunction *fun)
{
    return fun->nargs();
}

JS_PUBLIC_API(bool)
JS_FunctionHasLocalNames(JSContext *cx, JSFunction *fun)
{
    return fun->nonLazyScript()->bindings.count() > 0;
}

extern JS_PUBLIC_API(uintptr_t *)
JS_GetFunctionLocalNameArray(JSContext *cx, JSFunction *fun, void **memp)
{
    RootedScript script(cx, fun->nonLazyScript());
    BindingVector bindings(cx);
    if (!FillBindingVector(script, &bindings))
        return nullptr;

    LifoAlloc &lifo = cx->tempLifoAlloc();

    // Store the LifoAlloc::Mark right before the allocation.
    LifoAlloc::Mark mark = lifo.mark();
    void *mem = lifo.alloc(sizeof(LifoAlloc::Mark) + bindings.length() * sizeof(uintptr_t));
    if (!mem) {
        js_ReportOutOfMemory(cx);
        return nullptr;
    }
    *memp = mem;
    *reinterpret_cast<LifoAlloc::Mark*>(mem) = mark;

    // Munge data into the API this method implements.  Avert your eyes!
    uintptr_t *names = reinterpret_cast<uintptr_t*>((char*)mem + sizeof(LifoAlloc::Mark));
    for (size_t i = 0; i < bindings.length(); i++)
        names[i] = reinterpret_cast<uintptr_t>(bindings[i].name());

    return names;
}

extern JS_PUBLIC_API(JSAtom *)
JS_LocalNameToAtom(uintptr_t w)
{
    return reinterpret_cast<JSAtom *>(w);
}

extern JS_PUBLIC_API(JSString *)
JS_AtomKey(JSAtom *atom)
{
    return atom;
}

extern JS_PUBLIC_API(void)
JS_ReleaseFunctionLocalNameArray(JSContext *cx, void *mem)
{
    cx->tempLifoAlloc().release(*reinterpret_cast<LifoAlloc::Mark*>(mem));
}

JS_PUBLIC_API(JSScript *)
JS_GetFunctionScript(JSContext *cx, HandleFunction fun)
{
    if (fun->isNative())
        return nullptr;
    if (fun->isInterpretedLazy()) {
        AutoCompartment funCompartment(cx, fun);
        JSScript *script = fun->getOrCreateScript(cx);
        if (!script)
            MOZ_CRASH();
        return script;
    }
    return fun->nonLazyScript();
}

JS_PUBLIC_API(JSNative)
JS_GetFunctionNative(JSContext *cx, JSFunction *fun)
{
    return fun->maybeNative();
}

/************************************************************************/

JS_PUBLIC_API(JSFunction *)
JS_GetScriptFunction(JSContext *cx, JSScript *script)
{
    script->ensureNonLazyCanonicalFunction(cx);
    return script->functionNonDelazifying();
}

JS_PUBLIC_API(JSObject *)
JS_GetParentOrScopeChain(JSContext *cx, JSObject *obj)
{
    return obj->enclosingScope();
}

JS_PUBLIC_API(const char *)
JS_GetDebugClassName(JSObject *obj)
{
    if (obj->is<DebugScopeObject>())
        return obj->as<DebugScopeObject>().scope().getClass()->name;
    return obj->getClass()->name;
}

/************************************************************************/

JS_PUBLIC_API(const char *)
JS_GetScriptFilename(JSScript *script)
{
    return script->filename();
}

JS_PUBLIC_API(const jschar *)
JS_GetScriptSourceMap(JSContext *cx, JSScript *script)
{
    ScriptSource *source = script->scriptSource();
    JS_ASSERT(source);
    return source->hasSourceMapURL() ? source->sourceMapURL() : nullptr;
}

JS_PUBLIC_API(unsigned)
JS_GetScriptBaseLineNumber(JSContext *cx, JSScript *script)
{
    return script->lineno();
}

JS_PUBLIC_API(unsigned)
JS_GetScriptLineExtent(JSContext *cx, JSScript *script)
{
    return js_GetScriptLineExtent(script);
}

JS_PUBLIC_API(JSVersion)
JS_GetScriptVersion(JSContext *cx, JSScript *script)
{
    return VersionNumber(script->getVersion());
}

JS_PUBLIC_API(bool)
JS_GetScriptIsSelfHosted(JSScript *script)
{
    return script->selfHosted();
}

/***************************************************************************/

extern JS_PUBLIC_API(void)
JS_DumpPCCounts(JSContext *cx, HandleScript script)
{
    JS_ASSERT(script->hasScriptCounts());

    Sprinter sprinter(cx);
    if (!sprinter.init())
        return;

    fprintf(stdout, "--- SCRIPT %s:%d ---\n", script->filename(), (int) script->lineno());
    js_DumpPCCounts(cx, script, &sprinter);
    fputs(sprinter.string(), stdout);
    fprintf(stdout, "--- END SCRIPT %s:%d ---\n", script->filename(), (int) script->lineno());
}

JS_PUBLIC_API(void)
JS_DumpCompartmentPCCounts(JSContext *cx)
{
    for (ZoneCellIter i(cx->zone(), gc::FINALIZE_SCRIPT); !i.done(); i.next()) {
        RootedScript script(cx, i.get<JSScript>());
        if (script->compartment() != cx->compartment())
            continue;

        if (script->hasScriptCounts())
            JS_DumpPCCounts(cx, script);
    }

#if defined(JS_ION)
    for (unsigned thingKind = FINALIZE_OBJECT0; thingKind < FINALIZE_OBJECT_LIMIT; thingKind++) {
        for (ZoneCellIter i(cx->zone(), (AllocKind) thingKind); !i.done(); i.next()) {
            JSObject *obj = i.get<JSObject>();
            if (obj->compartment() != cx->compartment())
                continue;

            if (obj->is<AsmJSModuleObject>()) {
                AsmJSModule &module = obj->as<AsmJSModuleObject>().module();

                Sprinter sprinter(cx);
                if (!sprinter.init())
                    return;

                fprintf(stdout, "--- Asm.js Module ---\n");

                for (size_t i = 0; i < module.numFunctionCounts(); i++) {
                    jit::IonScriptCounts *counts = module.functionCounts(i);
                    DumpIonScriptCounts(&sprinter, counts);
                }

                fputs(sprinter.string(), stdout);
                fprintf(stdout, "--- END Asm.js Module ---\n");
            }
        }
    }
#endif
}

static const char *
FormatValue(JSContext *cx, const Value &vArg, JSAutoByteString &bytes)
{
    RootedValue v(cx, vArg);

    /*
     * We could use Maybe<AutoCompartment> here, but G++ can't quite follow
     * that, and warns about uninitialized members being used in the
     * destructor.
     */
    RootedString str(cx);
    if (v.isObject()) {
        AutoCompartment ac(cx, &v.toObject());
        str = ToString<CanGC>(cx, v);
    } else {
        str = ToString<CanGC>(cx, v);
    }

    if (!str)
        return nullptr;
    const char *buf = bytes.encodeLatin1(cx, str);
    if (!buf)
        return nullptr;
    const char *found = strstr(buf, "function ");
    if (found && (found - buf <= 2))
        return "[function]";
    return buf;
}

static char *
FormatFrame(JSContext *cx, const NonBuiltinScriptFrameIter &iter, char *buf, int num,
            bool showArgs, bool showLocals, bool showThisProps)
{
    JS_ASSERT(!cx->isExceptionPending());
    RootedScript script(cx, iter.script());
    jsbytecode* pc = iter.pc();

    RootedObject scopeChain(cx, iter.scopeChain());
    JSAutoCompartment ac(cx, scopeChain);

    const char *filename = script->filename();
    unsigned lineno = PCToLineNumber(script, pc);
    RootedFunction fun(cx, iter.maybeCallee());
    RootedString funname(cx);
    if (fun)
        funname = fun->atom();

    RootedValue thisVal(cx);
    if (iter.hasUsableAbstractFramePtr() && iter.computeThis(cx)) {
        thisVal = iter.computedThisValue();
    }

    // print the frame number and function name
    if (funname) {
        JSAutoByteString funbytes;
        buf = JS_sprintf_append(buf, "%d %s(", num, funbytes.encodeLatin1(cx, funname));
    } else if (fun) {
        buf = JS_sprintf_append(buf, "%d anonymous(", num);
    } else {
        buf = JS_sprintf_append(buf, "%d <TOP LEVEL>", num);
    }
    if (!buf)
        return buf;

    if (showArgs && iter.hasArgs()) {
        BindingVector bindings(cx);
        if (fun && fun->isInterpreted()) {
            if (!FillBindingVector(script, &bindings))
                return buf;
        }


        bool first = true;
        for (unsigned i = 0; i < iter.numActualArgs(); i++) {
            RootedValue arg(cx);
            if (i < iter.numFormalArgs() && script->formalIsAliased(i)) {
                for (AliasedFormalIter fi(script); ; fi++) {
                    if (fi.frameIndex() == i) {
                        arg = iter.callObj().aliasedVar(fi);
                        break;
                    }
                }
            } else if (script->argsObjAliasesFormals() && iter.hasArgsObj()) {
                arg = iter.argsObj().arg(i);
            } else {
                arg = iter.unaliasedActual(i, DONT_CHECK_ALIASING);
            }

            JSAutoByteString valueBytes;
            const char *value = FormatValue(cx, arg, valueBytes);

            JSAutoByteString nameBytes;
            const char *name = nullptr;

            if (i < bindings.length()) {
                name = nameBytes.encodeLatin1(cx, bindings[i].name());
                if (!buf)
                    return nullptr;
            }

            if (value) {
                buf = JS_sprintf_append(buf, "%s%s%s%s%s%s",
                                        !first ? ", " : "",
                                        name ? name :"",
                                        name ? " = " : "",
                                        arg.isString() ? "\"" : "",
                                        value ? value : "?unknown?",
                                        arg.isString() ? "\"" : "");
                if (!buf)
                    return buf;

                first = false;
            } else {
                buf = JS_sprintf_append(buf, "    <Failed to get argument while inspecting stack frame>\n");
                if (!buf)
                    return buf;
                cx->clearPendingException();

            }
        }
    }

    // print filename and line number
    buf = JS_sprintf_append(buf, "%s [\"%s\":%d]\n",
                            fun ? ")" : "",
                            filename ? filename : "<unknown>",
                            lineno);
    if (!buf)
        return buf;


    // Note: Right now we don't dump the local variables anymore, because
    // that is hard to support across all the JITs etc.

    // print the value of 'this'
    if (showLocals) {
        if (!thisVal.isUndefined()) {
            JSAutoByteString thisValBytes;
            RootedString thisValStr(cx, ToString<CanGC>(cx, thisVal));
            const char *str = nullptr;
            if (thisValStr &&
                (str = thisValBytes.encodeLatin1(cx, thisValStr)))
            {
                buf = JS_sprintf_append(buf, "    this = %s\n", str);
                if (!buf)
                    return buf;
            } else {
                buf = JS_sprintf_append(buf, "    <failed to get 'this' value>\n");
                cx->clearPendingException();
            }
        }
    }

    if (showThisProps && thisVal.isObject()) {
        RootedObject obj(cx, &thisVal.toObject());

        AutoIdVector keys(cx);
        if (!GetPropertyNames(cx, obj, JSITER_OWNONLY, &keys)) {
            cx->clearPendingException();
            return buf;
        }

        RootedId id(cx);
        for (size_t i = 0; i < keys.length(); i++) {
            RootedId id(cx, keys[i]);
            RootedValue key(cx, IdToValue(id));
            RootedValue v(cx);

            if (!JSObject::getGeneric(cx, obj, obj, id, &v)) {
                buf = JS_sprintf_append(buf, "    <Failed to fetch property while inspecting stack frame>\n");
                cx->clearPendingException();
                continue;
            }

            JSAutoByteString nameBytes;
            JSAutoByteString valueBytes;
            const char *name = FormatValue(cx, key, nameBytes);
            const char *value = FormatValue(cx, v, valueBytes);
            if (name && value) {
                buf = JS_sprintf_append(buf, "    this.%s = %s%s%s\n",
                                        name,
                                        v.isString() ? "\"" : "",
                                        value,
                                        v.isString() ? "\"" : "");
                if (!buf)
                    return buf;
            } else {
                buf = JS_sprintf_append(buf, "    <Failed to format values while inspecting stack frame>\n");
                cx->clearPendingException();
            }
        }
    }

    JS_ASSERT(!cx->isExceptionPending());
    return buf;
}

JS_PUBLIC_API(char *)
JS::FormatStackDump(JSContext *cx, char *buf, bool showArgs, bool showLocals, bool showThisProps)
{
    int num = 0;

    for (NonBuiltinScriptFrameIter i(cx); !i.done(); ++i) {
        buf = FormatFrame(cx, i, buf, num, showArgs, showLocals, showThisProps);
        num++;
    }

    if (!num)
        buf = JS_sprintf_append(buf, "JavaScript stack is empty\n");

    return buf;
}

JSAbstractFramePtr::JSAbstractFramePtr(void *raw, jsbytecode *pc)
  : ptr_(uintptr_t(raw)), pc_(pc)
{ }

JSObject *
JSAbstractFramePtr::scopeChain(JSContext *cx)
{
    AbstractFramePtr frame(*this);
    RootedObject scopeChain(cx, frame.scopeChain());
    AutoCompartment ac(cx, scopeChain);
    return GetDebugScopeForFrame(cx, frame, pc());
}

JSObject *
JSAbstractFramePtr::callObject(JSContext *cx)
{
    AbstractFramePtr frame(*this);
    if (!frame.isFunctionFrame())
        return nullptr;

    JSObject *o = GetDebugScopeForFrame(cx, frame, pc());

    /*
     * Given that fp is a function frame and GetDebugScopeForFrame always fills
     * in missing scopes, we can expect to find fp's CallObject on 'o'. Note:
     *  - GetDebugScopeForFrame wraps every ScopeObject (missing or not) with
     *    a DebugScopeObject proxy.
     *  - If fp is an eval-in-function, then fp has no callobj of its own and
     *    JS_GetFrameCallObject will return the innermost function's callobj.
     */
    while (o) {
        ScopeObject &scope = o->as<DebugScopeObject>().scope();
        if (scope.is<CallObject>())
            return o;
        o = o->enclosingScope();
    }
    return nullptr;
}

JSFunction *
JSAbstractFramePtr::maybeFun()
{
    AbstractFramePtr frame(*this);
    return frame.maybeFun();
}

JSScript *
JSAbstractFramePtr::script()
{
    AbstractFramePtr frame(*this);
    return frame.script();
}

bool
JSAbstractFramePtr::getThisValue(JSContext *cx, MutableHandleValue thisv)
{
    AbstractFramePtr frame(*this);

    RootedObject scopeChain(cx, frame.scopeChain());
    js::AutoCompartment ac(cx, scopeChain);
    if (!ComputeThis(cx, frame))
        return false;

    thisv.set(frame.thisValue());
    return true;
}

bool
JSAbstractFramePtr::isDebuggerFrame()
{
    AbstractFramePtr frame(*this);
    return frame.isDebuggerFrame();
}

bool
JSAbstractFramePtr::evaluateInStackFrame(JSContext *cx,
                                         const char *bytes, unsigned length,
                                         const char *filename, unsigned lineno,
                                         MutableHandleValue rval)
{
    if (!CheckDebugMode(cx))
        return false;

    size_t len = length;
    jschar *chars = InflateString(cx, bytes, &len);
    if (!chars)
        return false;
    length = (unsigned) len;

    bool ok = evaluateUCInStackFrame(cx, chars, length, filename, lineno, rval);
    js_free(chars);

    return ok;
}

bool
JSAbstractFramePtr::evaluateUCInStackFrame(JSContext *cx,
                                           const jschar *chars, unsigned length,
                                           const char *filename, unsigned lineno,
                                           MutableHandleValue rval)
{
    if (!CheckDebugMode(cx))
        return false;

    RootedObject scope(cx, scopeChain(cx));
    Rooted<Env*> env(cx, scope);
    if (!env)
        return false;

    AbstractFramePtr frame(*this);
    if (!ComputeThis(cx, frame))
        return false;
    RootedValue thisv(cx, frame.thisValue());

    js::AutoCompartment ac(cx, env);
    return EvaluateInEnv(cx, env, thisv, frame, mozilla::Range<const jschar>(chars, length),
                         filename, lineno, rval);
}

JSBrokenFrameIterator::JSBrokenFrameIterator(JSContext *cx)
{
    // Show all frames on the stack whose principal is subsumed by the current principal.
    NonBuiltinScriptFrameIter iter(cx,
                                   ScriptFrameIter::ALL_CONTEXTS,
                                   ScriptFrameIter::GO_THROUGH_SAVED,
                                   cx->compartment()->principals);
    data_ = iter.copyData();
}

JSBrokenFrameIterator::~JSBrokenFrameIterator()
{
    js_free((ScriptFrameIter::Data *)data_);
}

bool
JSBrokenFrameIterator::done() const
{
    NonBuiltinScriptFrameIter iter(*(ScriptFrameIter::Data *)data_);
    return iter.done();
}

JSBrokenFrameIterator &
JSBrokenFrameIterator::operator++()
{
    ScriptFrameIter::Data *data = (ScriptFrameIter::Data *)data_;
    NonBuiltinScriptFrameIter iter(*data);
    ++iter;
    *data = iter.data_;
    return *this;
}

JSAbstractFramePtr
JSBrokenFrameIterator::abstractFramePtr() const
{
    NonBuiltinScriptFrameIter iter(*(ScriptFrameIter::Data *)data_);
    return JSAbstractFramePtr(iter.abstractFramePtr().raw(), iter.pc());
}

jsbytecode *
JSBrokenFrameIterator::pc() const
{
    NonBuiltinScriptFrameIter iter(*(ScriptFrameIter::Data *)data_);
    return iter.pc();
}

bool
JSBrokenFrameIterator::isConstructing() const
{
    NonBuiltinScriptFrameIter iter(*(ScriptFrameIter::Data *)data_);
    return iter.isConstructing();
}
