'Import elements

'Based on interfaces published at:
'
'http://dev.w3.org/html5/spec/Overview.html
'http://dvcs.w3.org/hg/domcore/raw-file/tip/Overview.html
'http://dev.w3.org/2006/webapi/DOM-Level-3-Events/html/DOM3-Events.html
'http://simon.html5.org/dump/html5-canvas-cheat-sheet.html

#if TARGET<>"html5"
#Error "DOM module is only available for the html5 target"
#end

Import webgl
Import websocket

Import "dom.js"

Const ELEMENT_NODE=1
Const ATTRIBUTE_NODE=2
Const TEXT_NODE=3
Const CDATA_SECTION_NODE=4
Const ENTITY_REFERENCE_NODE=5
Const ENTITY_NODE=6
Const PROCESSING_INSTRUCTION_NODE=7
Const COMMENT_NODE=8
Const DOCUMENT_NODE=9
Const DOCUMENT_TYPE_NODE=10
Const DOCUMENT_FRAGMENT_NODE=11
Const NOTATION_NODE=12

Extern

Class DOMObject

	Method toString$()

End

Class Event Extends DOMObject

	Field type$
	Field target:EventTarget
	Field currentTarget:EventTarget
	Field eventPhase
	Field bubbles?
	Field cancelable?
	'Field timeStamp:DOMTimeStamp
	Field defaultPrevented?
	Field isTrusted?
	
	Method stopPropagation()
	Method PreventDefault()
	Method initEvent( eventTypeArg$,canBubbleArgb?,cancelableArg? )
	Method stopImmediatePropagation()
	
End

Class EventTarget Extends DOMObject

	Method addEventListener( type$,listener:EventListener,useCapture?=False )
	Method removeEventListener( type$,listener:EventListener,useCapture?=False )
	Method dispatchEvent( evt:Event )

End

Class EventListener Extends DOMObject

	Method handleEvent( event:Event )

End

Class Node Extends EventTarget

	Field nodeType
	Field nodeName$
	Field nodeValue$
	Field parentNode:Node
	Field childNodes:NodeList
	Field firstChild:Node
	Field lastChild:Node
	Field nextSibling:Node
	Field previousSibling:Node
	
	Method appendChild( child:Node )
	Method cloneNode:Node( deep? )
	Method hasChildNodes?()
	Method insertBefore( newChild:Node,refChild:Node )
	Method removeChild( child:Node )
	Method replaceChild( child:Node,oldChild:Node )
	Method getAttribute$( key$ )
	Method setAttribute( key$,value$ )

End

Class NodeList Extends DOMObject

	Field length
	
	Method item:Node( index )

End

Class Window Extends EventTarget

	'globals?
	'Field self_:Window
	'Field window:Window
	'Field document:HTMLDocument
	
	Field name$
	
	Field location:Location
	Field history:History
	
	Field locationbar:BarProp
	Field menubar:BarProp
	Field scrollbars:BarProp
	Field statusbar:BarProp
	Field toolbar:BarProp
	
	Field frames:Window
	Field length
	
	Field top:Window
	Field opener:Window
	Field parent:Window
	
	Field innerWidth
	Field innerHeight
	
	Field frameElement:Element

	'getter WindowProxy( index )
	'getter any( name$ )
	
	Field navigator:Navigator
	Field applicationCache:ApplicationCache
 
 	Method close()
	Method stop()
	Method focus()
	Method blur()
	Method open:Window( url$,target$,features$,replace$ )

	Method alert( message$ )
	Method confirm?( message$ )
	Method prompt$( message$,defaultText$ )
	Method print()
	Method showModalDialog:DOMObject( url$,arg0:DOMObject )
	Method eval:Object(expression$)
	
End

Class BarProp Extends DOMObject
	Field visible?
End

Class History Extends DOMObject
	Field length
	
	Method go( delta )
	Method back()
	Method forward()
	Method pushState( data:Object,title$,url$ )
	Method replaceState( data:Object,title$,url$ )
End

Class Location Extends DOMObject
	Field href$
	'
	Field protocol$
	Field host$
	Field hostname$
	Field port$
	Field pathname$
	Field search$
	Field hash$
	
	Method assign( url$ )
	Method replace( url$ )
	Method reload()
	Method resolveURL( url$ )
End
	


Class Navigator Extends DOMObject
	'interface NavigatorOnLine
	Field online?

	'interface NavigatorID
	Field appName$
	Field appVersion$
	Field platform$
	Field userAgent$
	
	'interface NavigatorContentutils
	Method registerProtocolHandler( scheme$,url$,title$ )
	Method registerContentHandler( scheme$,url$,title$ )
	
	'interface NavigatorStorageUtils
	Method yieldForStorageUpdates()
End

Class ApplicationCache Extends DOMObject
	'static
	'Const UNCACHED=0
	'Const IDLE=1
	'Const CHECKING=2
	'Const DOWNLOADING=3
	'Const UPDATEREADY=4
	'Const OBSOLETE=5
	
	Field status
	
	Method update()
	Method swapCache()
End

Global window:Window

Class HTMLCollection Extends DOMObject
	Field length

	Method item:Element( index )
	Method namedItem:Element( index )
End

Class Document Extends Node

	'Field implementation:DOMImplementation
	Field documentURI$
	Field compatMode$
	'Field doctype:DocumentType
	Field documentElement:Element
	
	Method getElementsByTagName:NodeList( qualifiedName$ )
	Method getElementsByTagNameNS:NodeList( namespace$,localName$ )
	Method getElementsByClassName:NodeList( classNames$ )
	Method getElementById:Element( elementId$ )

	Method createElement:Element( localName$ )
	Method createElementNS:Element( namespace$,qualifiedName$ )
	'Method createDocumentFragment:DocumentFragment()
	'Method createTextNode:Text( data$ )
	'Method createComment:Comment( data$ )
	'Method createProcessingInstruction:ProcessingInstruction( target$,data$ )
	
	Method importNode:Node( node:Node,deep? )
	Method adoptNode:Node( node:Node )
	
	Method createEvent( iface$ )

End

'According to docs, this isn't strictly correct.
'Document apparently Implements HTMLDocument which sounds a bit weird to me,
'I would have thought it was the other way around. This works for now anyway...
'
Class HTMLDocument Extends Document

	'Field location:Location
	
	Field URL$
	Field domain$
	Field referrer$
	Field cookie$
	Field lastModified$
	Field charset$
	Field characterSet$
	Field defaultCharset$
	Field readyState$
	
	Field title$
	Field dir$
	Field body:HTMLElement
	Field head:HTMLHeadElement
	
	Field images:HTMLCollection
	Field embeds:HTMLCollection
	Field plugins:HTMLCollection
	Field links:HTMLCollection
	Field forms:HTMLCollection
	Field scripts:HTMLCollection
	
	Field innerHTML$
	
	Field designMode$
	Field defaultView:Window
	Field activeElement:Element
	Field commands:HTMLCollection
	
	'special for some reason...?
	Field anchors:HTMLCollection
	Field applets:HTMLCollection
	'Field all:HTMLAllCollection
	
	'attrs
	Field fgColor$
	Field bgColor$
	Field linkColor$
	Field vlinkColor$
	Field alinkColor$

	Method open:HTMLDocument( type$,replace$ )
	Method open:Window( url$,name$,features$,replace? )
	Method close()
	Method write( text$ )
	Method writeln( text$ )
	
	Method getElementsByName:NodeList( elementName$ )
	Method getElementsByClassName:NodeList( classNames$ )
	
	Method hasFocus?()
	Method execCommand?( commandId$ )
	Method execCommand?( commandId$,showUI? )
	Method execCommand?( commandId$,showUI?,value$ )
	Method queryCommandEnabled?( commandId$ )
	Method queryCommandIndeterm?( commandId$ )
	Method queryCommandState?( commandId$ )
	Method queryCommandSupported?( commandId$ )
	Method queryCommandValue$( commandId$ )
	Method clear()
	
End

Global document:HTMLDocument

Class Element Extends Node
	
	Field namespaceURI$
	Field prefix$
	Field localName$
	Field tagName$
	'Field attributes:Attr[]()
	Field children:HTMLCollection
	Field firstElementChild:Element
	Field lastElementChild:Element
	Field previousElementSibling:Element
	Field nextElementSibling:Element
	
	Method getAttribute$( qualifiedName$ )
	Method getAttributeNS$( namespace$,localName$ )
	Method setAttribute( qualifiedName$,value$ )
	Method setAttributeNS( namespace$,localName$,value$ )
	Method removeAttribute( qualifiedName$ )
	Method removeAttributeNS( namespace$,localName$ )
	Method hasAttribute?( qualifiedName$ )
	Method hasAttributeNS?( namespace$,localName$ )
	
	Method getElementsByTagName:NodeList( qualifiedName$ )
	Method getElementsByTagNameNS:NodeList( namespace$,qualifiedName$ )
	Method getElementsByClassName:NodeList( classnames$ )
	
End

Class HTMLElement Extends Element

	Field innerHTML$
	Field outerHTML$
	
	Field id$
	Field title$
	Field lang$
	Field dir$
	Field className$
	'Field classList:DOMTokenList
	'Field dataset:DOMStringMap
	
	Field hidden?
	Field tabIndex

	Field accessKey$
	Field accesskeyLabel$
	Field draggable?
	'Field dropzone:DOMSettableTokenList
	Field contentEditable$
	Field isContentEditable?
	'Field contextMenu:HTMLMenuElement
	Field spellcheck?
	
	Field commandType$
	Field label$
	Field icon$
	Field disabled?
	Field checked?

	Field style:CSSStyleDeclaration
	
	Method getElementsByClassName:NodeList( classNames$ )
	Method insertAdjacentHTML( postion$,text$ )
	Method click()
	Method focus()
	Method blur()

End

Class HTMLHtmlElement Extends HTMLElement
	
	'attrs
	Field version$
End

Class HTMLHeadElement Extends HTMLElement
End

Class HTMLTitleElement Extends HTMLElement
	Field text$
End

Class HTMLBaseElement Extends HTMLElement
	Field href$
	Field target$
End

Class HTMLLinkElement Extends HTMLElement
	Field disabled?
	Field href$
	Field rel$
	'Field relList:DOMTokenList
	Field media$
	Field hreflang$
	Field type$
'	Field sizes:DomSettableTokenList
	
	'attrs
	Field charset$
	Field rev$
	Field target$
End

Class HTMLMetaElement Extends HTMLElement
	Field name$
	Field httpEquiv$
	Field content$
	
	'attrs
	Field scheme$
End

Class HTMLStyleElement Extends HTMLElement
	Field disabled?
	Field media$
	Field type$
	Field scope?
End

Class HTMLScriptElement Extends HTMLElement
	Field src$
	Field async?
	Field defer?
	Field type$
	Field charset$
	Field text$
	
	'attrs
	Field event$
	Field htmlFor$
End

Class HTMLBodyElement Extends HTMLElement
	
	'attrs
	Field text$
	Field bgColor$
	Field background$
	Field link$
	Field vLink$
	Field aLink$
End

Class HTMLHeadingElement Extends HTMLElement
	
	'attrs
	Field align$
End

Class HTMLParagraphElement Extends HTMLElement
	
	'attrs
	Field align$
End

Class HTMLHRElement Extends HTMLElement
	
	'attrs
	Field align$
	Field color$
	Field noShade?
	Field size$
	Field width$
End

Class HTMLPreElement Extends HTMLElement
	'
	Field with
End

Class HTMLQuoteElement Extends HTMLElement
	Field cite$
End

Class HTMLOListElement Extends HTMLElement
	Field reversed?
	Field start
	Field type$
	
	'attrs
	Field compact?
End

Class HTMLUListElement Extends HTMLElement
	Field compact?
	Field type$
End

Class HTMLLIElement Extends HTMLElement
	Field value
	
	'attrs
	Field type$
End

Class HTMLDListElement Extends HTMLElement
	
	'attrs
	Field compact?
End

Class HTMLDivElement Extends HTMLElement
	
	'attrs
	Field align$
End

Class HTMLAnchorElement Extends HTMLElement
	Field href$
	Field target$
	Field rel$
'	Field relList:DOMTokenList
	Field media$
	Field hreflang$
	Field type$
	Field text$
	'
	Field protocol$
	Field host$
	Field hostname$
	Field port$
	Field pathname$
	Field search$
	Field hash$
	
	'attrs
	Field coords$
	Field charset$
	Field name$
	Field rev$
	Field shape$
End

Class HTMLTimeElement Extends HTMLElement
	Field dateTime$
	Field pubDate?
'	Field valueAsDate:Date
End

Class HTMLSpanElement Extends HTMLElement
End

Class HTMLBRElement Extends HTMLElement
	
	'attrs
	Field clear$
End

Class HTMLModElement Extends HTMLElement
	Field cite$
	Field dateTime$
End

Class HTMLImageElement Extends HTMLElement
	Field alt$
	Field src$
	Field useMap$
	Field isMap?
	Field width
	Field height
	Field naturalWidth
	Field naturalHeight
	Field complete?
	
	'attrs
	Field name$
	Field align$
	Field border$
	Field hspace
	Field longDesc$
	Field vspace
End

Class HTMLIFrameElement Extends HTMLElement
	Field src$
	Field srcdoc$
	Field name$
'	Field sandbox:DOMSettableTokenList
	Field seamless?
	Field width$
	Field height$
	Field contentDocument:HTMLDocument
	Field contentWindow:Window
	
	'attrs
	Field align$
	Field frameBorder$
	Field longDesc$
	Field marginHeight$
	Field marginWidth$
	Field scrolling$
	
End

Class HTMLEmbedElement Extends HTMLElement
	Field src$
	Field type$
	Field width$
	Field height$
	
	'attrs
	Field align$
	Field name$
End

Class HTMLObjectElement Extends HTMLElement
	Field data$
	Field type$
	Field name$
	Field useMap$
	Field form:HTMLFormElement
	Field width$
	Field height$
	Field contentDocument:HTMLDocument
	Field contentWindow:Window

	Field willValidate?
'	Field validity:ValidityState
	Field validationMessage$
	
	'attrs
	Field align$
	Field archive$
	Field border$
	Field code$
	Field codeBase$
	Field codeType$
	Field declare?
	Field hspace
	Field standby$
	Field vspace

	Method checkValidity?()
	Method setCustomValidity( error$ )	
End

Class HTMLParamElement Extends HTMLElement
	Field name$
	Field value$
	
	'attrs
	Field type$
	Field valueType$
End

Class HTMLVideoElement Extends HTMLMediaElement
	Field width
	Field height
	Field videoWidth
	Field videoHeight
	Field poster$
'	Field audio:DOMSettableTokenList
End

Class HTMLSourceElement Extends HTMLElement
	Field src$
	Field type$
	Field media$
End

Class HTMLTrackElement Extends HTMLElement
	Field kind$
	Field src$
	Field srclang$
	Field label$
	Field default_?
'	Field track:TextTrack

End

Class HTMLMediaElement Extends HTMLElement
'	Field error:MediaError

	'networkState
	'Const NETWORK_EMPTY=0
	'Const NETWORK_IDLE=1
	'Const NETWORK_LOADING=2
	'Const NETWORK_NO_SOURCE=3
	
	'readyState
	'Const HAVE_NOTHING=0
	'Const HAVE_METADATA=1
	'Const HAVE_CURRENT_DATA=2
	'Const HAVE_FUTURE_DATA=3
	'Const HAVE_ENOUGH_DATA=4
	
	Field src$
	Field currentSrc$
	Field networkState
	Field preload$
'	Field buffered:TimeRanges
	Field readyState
	Field seeking?
	
	Field currentTime#
	Field initialTime#
	Field duration#
'	Field startOffsetTime:Date
	Field paused?
	Field defaultPlaybackRate#
	Field playbackRate#
'	Field played:TimeRanges
'	Field seekable:TimeRanges
	Field ended?
	Field autoplay?
	Field loop?
	Field controls?
	Field volume#
	Field muted?
'	Field textTracks:TextTrack[]
			
	Method canPlayType$( type$ )
	Method load()
	Method play()
	Method Pause()
'	Method addTrack:MutableTextTrack( kind$,label$,language$ )

End

Class HTMLAudioElement Extends HTMLMediaElement

End

Class HTMLCanvasElement Extends HTMLElement
	Field width
	Field height
	
'	Method toDataURL$( type$,args... )

	Method getContext:Object( contextId$ )
	Method getContext:Object( contextId$,arg0:Object )	
	Method getContext:Object( contextId$,arg0:Object,arg1:Object )	

End

Class HTMLMapElement Extends HTMLElement
	Field name$
	Field areas:HTMLCollection
	Field images:HTMLCollection
End

Class HTMLAreaElement Extends HTMLElement
	Field alt$
	Field coords$
	Field shape$
	Field href$
	Field target$
	Field rel$
'	Field relList:DOMTokenList
	Field media$
	Field hreflang$
	Field type$
	'
	Field protocol$
	Field host$
	Field hostname$
	Field port$
	Field pathname$
	Field search$
	Field hash$
	
	'attrs
	Field noHref?
End

Class HTMLTableElement Extends HTMLElement
	Field caption:HTMLTableCaptionElement
	Field tHead:HTMLTableSectionElement
	Field tFoot:HTMLTableSectionElement
	Field tBodies:HTMLCollection
	Field rows:HTMLCollection
	Field summary$
	
	Method createCaption:HTMLElement()
	Method deleteCaption()
	Method createTHead:HTMLElement()
	Method deleteTHead()
	Method createTFoot:HTMLElement()
	Method deleteTFoot()
	Method createTBody:HTMLElement()
	Method insertRow:HTMLElement( index )
	Method deleteRow( index )

	'attrs
	Field align$
	Field bgColor$
	Field border$
	Field cellPadding$
	Field cellSpacing$
	Field frame$
	Field rules$
	Field width$
End

Class HTMLTableCaptionElement Extends HTMLElement

	'attrs
	Field align$
End

Class HTMLTableColElement Extends HTMLElement
	Field span

	'attrs
	Field align$
	Field ch$
	Field chOff$
	Field vAlign$
	Field width$
End

Class HTMLTableSectionElement Extends HTMLElement
	Field rows:HTMLCollection
	
	Method insertRow:HTMLElement( index )
	Method deleteRow( index )

	'attrs
	Field align$
	Field ch$
	Field chOff$
	Field vAlign$
End

Class HTMLTableRowElement Extends HTMLElement
	Field rowIndex
	Field sectionRowIndex
	
	Method insertCell:HTMLElement( index )
	Method deleteCell( index )
	
	'attrs
	Field align$
	Field bgColor$
	Field ch$
	Field chOff$
	Field vAlign$
End

Class HTMLTableCellElement Extends HTMLElement
	Field colSpan
	Field rowSpan
'	Field headers:DOMSettableTokenList
	Field cellIndex
	
	'attrs
	Field abbr$
	Field align$
	Field axis$
	Field bgColor$
	Field ch$
	Field chOff$
	Field height$
	Field noWrap?
	Field vAlign$
	Field width$
End

Class HTMLTableDataCellElement Extends HTMLTableCellElement
End

Class HTMLTableHeaderCellElement Extends HTMLTableCellElement
	Field scope$
End

Class HTMLFormElement Extends HTMLElement
	Field acceptCharset$
	Field action$
	Field autocomplete$
	Field enctype$
	Field encoding$
	Field method_$
	Field name$
	Field noValidate?
	Field target$
	
'	Field elements:HTMLFormControlsCollection

	Method submit()
	Method reset()
	Method checkValidity?()
End

Class HTMLFieldSetElement Extends HTMLElement
	Field disabled?
	Field form:HTMLFormElement
	Field name$
	Field type$
	
'	Field elements:HTMLFormControlsCollection

	Field willValidate?
'	Field validity:ValidityState
	Field validationMessage$
	
	Method checkValidity?()
	Method setCustomValidity( error$ )
End

Class HTMLLegendElement Extends HTMLElement
	Field form:HTMLFormElement
	
	'attrs
	Field align$
End

Class HTMLLabelElement Extends HTMLElement
	Field form:HTMLFormElement
	Field htmlFor$
	Field control:HTMLElement
End

Class HTMLInputElement Extends HTMLElement
	Field accept$
	Field alt$
	Field autocomplete$
	Field autofocus?
	Field defaultChecked?
	Field checked?
	Field dirName$
	Field disabled?
	Field form:HTMLFormElement
'	Field files:FileList
	Field formAction$
	Field formEnctype$
	Field formMethod$
	Field formNoValidate?
	Field formTarget$
	Field height$
	Field indeterminate?
	Field list:HTMLElement
	Field max$
	Field maxLength
	Field min$
	Field multiple?
	Field name$
	Field pattern$
	Field placeholder$
	Field readOnly?
	Field required?
	Field size
	Field src$
	Field step_$
	Field type$
	Field defaultValue$
	Field value$
'	Field valueAsDate:Date
	Field valueAsNumber#
	Field selectedOption:HTMLOptionElement
	Field width$
	
	Field willValidate?
'	Field validity:ValidityState
	Field validationMessage$
	
	Field labels:NodeList
	
	Field selectionStart
	Field selectionEnd
	
	'attrs
	Field align$
	Field useMap$
	
	Method stepUp( n )
	Method stepDown( n )
	
	Method checkValidity()
	Method setCustomValidity( error$ )
	
	Method select_()
	
	Method setSelectionRange( start,end_ )
End

Class HTMLButtonElement Extends HTMLElement
	Field autofocus?
	Field disabled?
	Field form:HTMLFormElement
	Field formAction$
	Field formEnctype$
	Field formMethod$
	Field formNoValidate?
	Field formTarget$
	Field name$
	Field type$
	Field value$

	Field willValidate?
'	Field validity:ValidityState
	Field validationMessage$
	
	Field labels:NodeList
	
	Method checkValidity()
	Method setCustomValidity( error$ )
	
End

Class HTMLSelectElement Extends HTMLElement
	Field autofocus?
	Field disabled?
	Field form:HTMLFormElement
	Field multiple?
	Field name$
	Field required?
	Field size
	
	Field type$
	
	'Field options:HTMLOptionsCollection
	Field length
	Method item:Object( index )
	Method namedItem:Object( name$ )
	
	Field selectedOptions:HTMLCollection
	Field selectedIndex
	Field value$
	
	Field willValidate?
'	Field validity:ValidityState
	Field validationMessage$
	
	Field labels:NodeList
	
	Method add( element:HTMLElement )	
	Method add( element:HTMLElement,before )	
	Method add( element:HTMLElement,before:HTMLElement )
	
	Method remove( index )
	
	Method checkValidity()
	Method setCustomValidity( error$ )

End

Class HTMLDataListElement Extends HTMLElement
	Field options:HTMLCollection
End

Class HTMLOptGroupElement Extends HTMLElement
	Field disabled?
	Field label$
End

Class HTMLOptionElement Extends HTMLElement
	Field disabled?
	Field form:HTMLFormElement
	Field label$
	Field defaultSelected?
	Field selected?
	Field value$
	
	Field text$
	Field index
End

Class HTMLTextAreaElement Extends HTMLElement
	Field autofocus?
	Field cols
	Field dirName$
	Field disabled?
	Field form:HTMLFormElement
	Field maxLength
	Field name$
	Field placeholder$
	Field readOnly?
	Field required?
	Field rows
	Field wrap$
	
	Field type$
	Field DefaultValue$
	Field value$
	Field textlength
	
	Field willValidate?
'	Field validity:ValidityState
	Field validationMessage$
	
	Field labels:NodeList
	
	Field selectionStart
	Field selectionEnd
	
	Method select_()	
	Method setSelectionRange( start,end_ )

	Method checkValidity()
	Method setCustomValidity( error$ )
End

Class HTMLKeygenElement Extends HTMLElement
	Field audiofocus?
	Field challenge$
	Field disabled?
	Field form:HTMLFormElement
	Field keyttype$
	Field name$
	
	Field type$
	
	Field willValidate?
'	Field validity:ValidityState
	Field validationMessage$
	
	Field labels:NodeList
	
	Method checkValidity()
	Method setCustomValidity( error$ )
End

Class HTMLOutputElement Extends HTMLElement
	'Field htmlFor:DOMSettableTokenList
	Field form:HTMLFormElement
	Field name$
	
	Field type$
	Field defaultValue$
	Field value$
	
	Field willValidate?
'	Field validity:ValidityState
	Field validationMessage$
	
	Field labels:NodeList
	
	Method checkValidity()
	Method setCustomValidity( error$ )
End
	
Class HTMLProgressElement Extends HTMLElement
	Field value#
	Field max#
	Field position#
	Field form:HTMLFormElement
	Field labels:NodeList
End

Class HTMLMeterElement Extends HTMLElement
	Field value#
	Field min#
	Field max#
	Field low#
	Field high#
	Field optimum#
	Field form:HTMLFormElement
	Field labels:NodeList
End
	
Class HTMLDetailsElement Extends HTMLElement
	Field open?
End

Class HTMLCommandElement Extends HTMLElement
	Field type$
	Field label$
	Field icon$
	Field disabled?
	Field checked?
	Field radiogroup$
End

Class HTMLMenuElement Extends HTMLElement
	Field type$
	Field label$
	
	'attrs
	Field compact?
End

Class HTMLAppletElement Extends HTMLElement
	Field align$
	Field alt$
	Field archive$
	Field code$
	Field codeBase$
	Field height$
	Field hspace
	Field name$
	Field object_$="object"
	Field vspace
	Field width$
End

Class HTMLMarqueeElement Extends HTMLElement
	Field behavior$
	Field bgColor$
	Field direction$
	Field height$
	Field hspace
	Field loop
	Field scrollAmount
	Field scrollDelay
	Field trueSpeed?
	Field vspace
	Field width$
	
	Method start()
	Method stop()
End

Class HTMLFrameSetElement Extends HTMLElement
	Field cols$
	Field rows$
End

Class HTMLFrameElement Extends HTMLElement
	Field frameBorder$
	Field longDesc$
	Field marginWidth$
	Field marginHeight$
	Field name$
	Field noResize?
	Field scrolling$
	Field src$
	Field contentDocument:HTMLDocument
	Field contentWindow:Window
End

Class HTMLFontElement Extends HTMLElement
	
	'attrs
	Field color$
	Field face$
	Field size$
End

Class HTMLBaseFontElement Extends HTMLElement
	
	'attrs
	Field color$
	Field face$
	Field size
End

Class HTMLDirectoryElement Extends HTMLElement
	Field compact?
End

'this is from: http://simon.html5.org/dump/html5-canvas-cheat-sheet.html

Class CanvasRenderingContext2D Extends DOMObject

	Field canvas:HTMLCanvasElement
	
	Field globalAlpha#
	Field globalCompositeOperation$
	
	Field strokeStyle$
	Field fillStyle$

	Field lineWidth#
	Field lineCap$
	Field lineJoin$
	Field miterLimit$
	
	Field shadowOffsetX#
	Field shadowOffsetY#
	Field shadowBlur#
	Field shadowColor$
	
	Field font$
	Field textAlign$
	Field textBaseline$
	
	Method save()
	Method restore()
	
	Method scale( x#,y# )
	Method rotate( angle# )
	Method translate( x#,y# )
	Method transform( m11#,m12#,m21#,m22#,dx#,dy# )
	Method setTransform( m11#,m12#,m21#,m22#,dx#,dy# )
	
	Method drawImage( image:Object,dx#,dy# )
	Method drawImage( image:Object,dx#,dy#,dw#,dh# )
	Method drawImage( image:Object,sx#,sy#,sw#,sh#,dx#,dy#,dw#,dh# )	

	Method createLinearGradient:CanvasGradient( x0#,y0#,x1#,y1# )
	Method createRadialGradient:CanvasGradient( x0#,y0#,r0#,x1#,y1#,r1# )
	Method createPattern:CanvasPattern( image:Object,repetition$ )

	Method beginPath()
	Method closePath()
	Method fill()
	Method stroke()
	Method clip()
	Method moveTo( x#,y# )
	Method lineTo( x#,y# )
	Method quadraticCurveTo( cpx#,cpy#,x#,y# )
	Method bezierCurveTo( cp1x#,cp1y#,cp2x#,cp2y#,x#,y# )
	Method arcTo( x1#,y1#,x2#,y2#,radius# )
	Method arc( x#,y#,radius#,startAngle#,endAngle#,anticlockwise? )
	Method rect( x#,y#,w#,h# )
	Method isPointInPath?( x#,y# )
	
	Method fillText( text$,x#,y# )
	Method fillText( text$,x#,y#,maxWidth# )
	Method strokeText( text$,x#,y# )
	Method strokeText( text$,x#,y#,maxWidth# )
	Method measureText:TextMetrics( text$ )
	
	Method clearRect( x#,y#,w#,h# )
	Method fillRect( x#,y#,w#,h# )
	Method strokeRect( x#,y#,w#,h# )
	
	Method createImageData:ImageData( sw#,sh# )
	Method getImageData:ImageData( sx#,sy#,sw#,sh# )
	Method putImageData( imagedata:ImageData,dx#,dy# )
	Method putImageData( imagedata:ImageData,dx#,dy#,dirtyX#,dirtyY#,dirtyWidth#,dirtyHeight# )

End

Class CanvasGradient Extends DOMObject
	Method addColorStop( offset#,color$ )
End

Class CanvasPattern Extends DOMObject
End

Class TextMetrics Extends DOMObject
	Field width#
End

Class ImageData Extends DOMObject
	Field width
	Field height
	Field data:Int[]	'CanvasPixelArray
End

Class CanvasPixelArray Extends DOMObject
	Field length
End

'This from skidracer!

Class CSSStyleDeclaration
	Field azimuth$
	Field background$
	Field backgroundAttachment$
	Field backgroundColor$
	Field backgroundImage$
	Field backgroundPosition$
	Field backgroundRepeat$
	Field border$
	Field borderCollapse$
	Field borderColor$
	Field borderSpacing$
	Field borderStyle$
	Field borderTop$
	Field borderRight$
	Field borderBottom$
	Field borderLeft$
	Field borderTopColor$
	Field borderRightColor$
	Field borderBottomColor$
	Field borderLeftColor$
	Field borderTopStyle$
	Field borderRightStyle$
	Field borderBottomStyle$
	Field borderLeftStyle$
	Field borderTopWidth$
	Field borderRightWidth$
	Field borderBottomWidth$
	Field borderLeftWidth$
	Field borderWidth$
	Field bottom$
	Field captionSide$
	Field clear$
	Field clip$
	Field color$
	Field content$
	Field counterIncrement$
	Field counterReset$
	Field cue$
	Field cueAfter$
	Field cueBefore$
	Field cursor$	
	Field direction$
	Field display$
	Field elevation$
	Field emptyCells$
	Field cssFloat$
	Field font$
	Field fontFamily$
	Field fontSize$
	Field fontSizeAdjust$
	Field fontStretch$
	Field fontStyle$
	Field fontVariant$
	Field fontWeight$
	Field height$
	Field left$
	Field letterSpacing$	
	Field lineHeight$		
	Field listStyle$
	Field listStyleImage$
	Field listStylePosition$
	Field listStyleType$
	Field margin$
	Field marginTop$
	Field marginRight$
	Field marginBottom$
	Field marginLeft$
	Field markerOffset$
	Field marks$
	Field maxHeight$
	Field maxWidth$
	Field minHeight$
	Field minWidth$
	Field orphans$
	Field outline$
	Field outlineColor$
	Field outlineStyle$
	Field outlineWidth$
	Field overflow$
	Field padding$
	Field paddingTop$
	Field paddingRight$
	Field paddingBottom$
	Field paddingLeft$
	Field page$	
	Field pageBreakAfter$	
	Field pageBreakBefore$	
	Field pageBreakInside$	
	Field pause$
	Field pauseAfter$
	Field pauseBefore$
	Field pitch$
	Field pitchRange$
	Field playDuring$
	Field position$
	Field quotes$
	Field richness$
	Field right$
	Field size$
	Field speak$
	Field speakHeader$
	Field speakNumeral$
	Field speakPunctuation$
	Field speechRate$
	Field stress$
	Field tableLayout$
	Field textAlign$
	Field textDecoration$
	Field textIndent$
	Field textShadow$
	Field textTransform$
	Field top$
	Field unicodeBidi$
	Field verticalAlign$
	Field visibility$
	Field voiceFamily$
	Field volume$
	Field whiteSpace$
	Field widows$
	Field width$	
	Field wordSpacing$
	Field zIndex$
End

