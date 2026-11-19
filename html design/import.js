// Parse CSV with quoted cells, commas, CRLF, and escaped double-quotes.
function parseCSV(text) {
  const rows=[]; let row=[], cell='', quote=false;
  for(let i=0;i<text.length;i++) {
    const ch=text[i];
    if(quote) {
      if(ch==='"' && text[i+1]==='"'){cell+='"';i++;}
      else if(ch==='"')quote=false;
      else cell+=ch;
    } else if(ch==='"')quote=true;
    else if(ch===','){row.push(cell);cell='';}
    else if(ch==='\n'){row.push(cell);rows.push(row);row=[];cell='';}
    else if(ch==='\r') { if(text[i+1]==='\n')continue; row.push(cell);rows.push(row);row=[];cell=''; }
    else cell+=ch;
  }
  if(quote)throw Error('Unclosed quote in CSV.');
  if(cell.length || row.length){row.push(cell);rows.push(row);}
  const nonempty=rows.filter(r=>r.some(x=>x.trim()));
  if(nonempty.length===0)throw Error('CSV is empty.');
  const head=nonempty[0].map(x=>x.trim().toLowerCase().replace(/^﻿/,''));
  const q=head.indexOf('question'), a=head.indexOf('answer');
  if(q<0 || a<0)throw Error('CSV needs question,answer headers.');
  return nonempty.slice(1).map(r=>({question:(r[q]||'').trim(),acceptedAnswers:[(r[a]||'').trim()]}));
}
function parseJSON(text) {
  const parsed=JSON.parse(text);
  const entries=Array.isArray(parsed)?parsed:parsed?.cards;
  if(!Array.isArray(entries))throw Error('JSON must contain an array of cards.');
  return entries.map(x=>({question:x?.question,acceptedAnswers:x?.acceptedAnswers??[x?.answer]}));
}
document.getElementById('importButton').addEventListener('click',async()=>{
  const message=document.getElementById('message');
  const btn=document.getElementById('importButton');
  const file=document.getElementById('importFile').files[0];
  if(!file){message.textContent='Please select a file first.';return;}
  const token=sessionStorage.getItem('token');
  if(!token){location.href='/main';return;}
  btn.disabled=true; message.textContent='Importing...';
  try {
    if(file.size>1024*1024)throw Error('File must be under 1 MB.');
    const text=await file.text();
    const cards=file.name.toLowerCase().endsWith('.csv')?parseCSV(text):
      file.name.toLowerCase().endsWith('.json')?parseJSON(text):null;
    if(!cards)throw Error('Only CSV and JSON files are supported.');
    if(cards.length===0 || cards.length>500)throw Error('Import must contain 1–500 cards.');
    for(let i=0;i<cards.length;i++) {
      const card=cards[i];
      if(typeof card.question!=='string'||!card.question.trim() ||card.question.length>1000 ||
        !Array.isArray(card.acceptedAnswers)||card.acceptedAnswers.length<1 ||card.acceptedAnswers.length>20 ||
        card.acceptedAnswers.some(a=>typeof a!=='string'||!a.trim()||a.length>1000))
        throw Error(`Invalid question or answer on card ${i+1}.`);
    }
    const response=await fetch('/api/cards/import',{
      method:'POST',headers:{'Authorization':'Bearer '+token,'Content-Type':'application/json'},
      body:JSON.stringify({cards})
    });
    if(response.status===401){sessionStorage.removeItem('token');location.href='/main';return;}
    if(!response.ok)throw Error(await response.text()||'Server rejected import.');
    const result=await response.json();
    message.textContent=`Imported ${result.imported} card(s). Select Play to study them.`;
  }catch(e){message.textContent='Import failed: '+e.message;}
  finally{btn.disabled=false;}
});
