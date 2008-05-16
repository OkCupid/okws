<?php 

//
// purify.php
//
// This is a simple PHP script  that can be called from XML-RPC
// to purify HTML input.  To run it, you'll have to:
//
//  (1) Install the HTML purifier:  
//
//   pear channel-discover htmlpurifier.org
//   pear install hp/HTMLPurifier
//
//  (2) install libraries for XMl-RPC servers, just copying
//      xmlrpc.inc and xmlrpcs.inc into place.
//
// 

require_once '/usr/share/php/HTMLPurifier.auto.php' ;
require_once "/usr/share/php/xmlrpc.inc";
require_once "/usr/share/php/xmlrpcs.inc";

class html {
	function purify ($xmlrpcmsg) {
		$purifier = new HTMLPurifier ();
		$in = $xmlrpcmsg->getParam (0)->scalarVal ();
		$out = new xmlrpcval ($purifier->purify ($in), "base64");
		return new xmlrpcresp ($out);
	}
}

$purify_doc = "sanitize HTML strings using HTML/Purifier";

$a = new xmlrpc_server(
	array (
		"html.purify" => array ("function" => "html::purify",
								"docstring" => $purify_doc )
	)
);

?>
