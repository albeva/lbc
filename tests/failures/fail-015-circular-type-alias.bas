''------------------------------------------------------------------------------
'' Check error message for circular type dependencies
''
'' CHECK: __FILE__:8:6: error: Circular type dependency detected on 'TFOO'
'' CHECK: type TFoo as typeof(TTFoo)
'' CHECK: ~~~~~^~~~~~~~~~~~~~~~~~~~~
''------------------------------------------------------------------------------
type TFoo as typeof(TTFoo)
type TTFoo as typeof(TFoo)