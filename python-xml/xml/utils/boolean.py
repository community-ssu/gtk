
true = True
false = False

BooleanType = bool

def BooleanValue(obj, func):
    if isinstance(obj, bool):
        return obj
    if isinstance(obj, basestring):
        return bool(func(obj))
    return bool(obj)

IsBooleanType = lambda obj: isinstance(obj, bool)
