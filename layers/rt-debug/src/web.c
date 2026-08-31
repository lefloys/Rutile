#include "web.h"

const char* rtdbg_web_page(void) {
	static const char page[] =
		"<!doctype html><meta charset=utf-8><title>rt-debug</title>"
		"<style>body{margin:0;background:#101419;color:#d7e1ea;font:14px ui-monospace,Consolas,monospace}header{padding:14px 20px;background:#18212b}button{margin-right:8px;padding:6px 12px;background:#263747;color:#fff;border:1px solid #50677b;border-radius:3px}#state{margin-left:14px}main{padding:12px}section{background:#17202a;padding:12px}h2{font-size:14px;margin:0 0 10px;color:#9fc6e8}img{display:block;max-width:100%;max-height:calc(100vh - 125px);margin:auto;image-rendering:pixelated}</style>"
		"<header><strong>rt-debug</strong><button onclick=control('pause')>Pause</button><button onclick=control('step')>Step</button><button onclick=control('continue')>Continue</button><span id=state></span></header>"
		"<main><section><h2>Current software frame</h2><img id=frame alt='No completed software frame.'></section></main>"
		"<script>let displayedFrame;function control(action){fetch('/control?'+action)}async function poll(){try{let state=await(await fetch('/events')).json();window.state.textContent=state.paused?'paused':'running';if(state.software_frame&&state.software_frame!==displayedFrame){displayedFrame=state.software_frame;window.frame.src='/texture?id=18446744073709551615&frame='+displayedFrame}}catch(error){window.close();window.state.textContent='connection ended';return}setTimeout(poll,100)}poll()</script>";
	return page;
}
