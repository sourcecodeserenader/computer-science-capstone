from pathlib import Path
import subprocess,os,re,json
r=Path(__file__).resolve().parent
env=os.environ.copy();env.update(CS499_USERNAME="review_user",CS499_PASSWORD="ReviewPass2026")
cases=[
("exact_id","5\n1\n2\n8\n12\n",["Sarah Davis"],[2]),
("missing_id","5\n1\n999\n8\n12\n",["No client found"],[]),
("prefix","5\n3\nsa\n8\n12\n",["Prefix Search Results"],[2]),
("duplicate_name","2\nBob Jones\n1\n33\n1000\n2\nbob.test@example.com\n5550100\ny\n5\n2\nBob Jones\n8\n12\n",["Exact Name Matches"],[1,6]),
("top_k","8\n1\n2\n12\n",["Client Rankings"],[5,4]),
("undo_update","3\n1\n1\nRobert Test\n9\n10\n5\n1\n1\n8\n12\n",["Undo completed","Bob Jones"],[1]),
("undo_delete","4\n1\ny\n10\n5\n1\n1\n8\n12\n",["Undo completed","Bob Jones"],[1]),
("index_rebuild","11\n2\n1\n4\n12\n",["rebuilt successfully","Index status: VALID","Indexed client references: 5"],[]),
("sort_name","7\n2\n12\n",["Sorted Client View"],[3,1,5,4,2]),
("filter_retirement","6\n2\n10\n12\n",["Retirement Clients"],[2,5]),
("analytics","9\n12\n",["Total assets: $1411750.00","Median balance: $286400.00"],[])
]
results=[]
for name,commands,strings,expected in cases:
 folder=r/("algorithm_"+name);folder.mkdir(exist_ok=True)
 p=subprocess.run([str(r/"milestone3.exe")],input="review_user\nReviewPass2026\n"+commands,text=True,capture_output=True,cwd=folder,env=env,timeout=8)
 (folder/"console.txt").write_text(p.stdout+p.stderr)
 ids=[int(x) for x in re.findall(r"^\s*(\d+)\s+[^\n]+(?:Brokerage|Retirement)\s+\d+",p.stdout,re.M)]
 ok=p.returncode==0 and all(x in p.stdout for x in strings) and (ids==expected if expected else not ids)
 if name=="filter_retirement":ok=p.returncode==0 and all(x in p.stdout for x in strings) and set(ids)==set(expected)
 results.append(dict(case=name,passed=ok,expected_ids=expected,actual_ids=ids))
 print("PASS" if ok else "FAIL",name,"expected",expected,"actual",ids,flush=True)
(r/"algorithm_regression_results.json").write_text(json.dumps(results,indent=2))
assert all(x["passed"] for x in results)
