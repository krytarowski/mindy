find-dylan-files /compiler/src | etags -l none -r "/define \(\(sealed\|open\|abstract\|concrete\|primary\|free\|inline\|movable\|flushable\|functional\|/\*\s*exported\s*\*/\) \)*\(method\|generic\|function\|class\|variable\|constant\) \([-A-Za-z0-9!&*<>|^$%@_?=]+\)/" -r "/[ \t]*\(\(virtual\|constant\|sealed\|instance\|class\|each-subclass\|/\*\s*exported\s*\*/\) \)*slot \([-A-Za-z0-9!&*<>|^$%@_?=]+\)/" -
