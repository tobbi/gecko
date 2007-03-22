/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is JavaScript Engine testing utilities.
 *
 * The Initial Developer of the Original Code is
 * Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): Seno Aiko
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

START("Function.prototype.toString should not quote XML literals");

var bug = 301692;
var summary = 'Function.prototype.toString should not quote XML literals';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

actual = ToString((function () { return <xml/>; }));
expect = 'function () { return <xml/>;}';
TEST(1, expect, actual);

actual = ToString((function () { return <xml></xml>; }));
expect = 'function () { return <xml></xml>;}';
TEST(2, expect, actual);

actual = ToString((function () { return <xml><morexml/></xml>; }));
expect = 'function () { return <xml><morexml/></xml>;}';
TEST(3, expect, actual);

actual = ToString((function (k) { return <xml>{k}</xml>; }));
expect = 'function (k) { return <xml>{k}</xml>;}';
TEST(4, expect, actual);

actual = ToString((function (k) { return <{k}/>; }));
expect = 'function (k) { return <{k}/>;}';
TEST(5, expect, actual);

actual = ToString((function (k) { return <{k}>{k}</{k}>; }));
expect = 'function (k) { return <{k}>{k}</{k}>;}';
TEST(6, expect, actual);

actual = ToString((function (k) { return <{k} 
                  {k}={k} {"k"}={k + "world"}><{k + "k"}/></{k}>; }));
expect = 'function (k) ' + 
         '{ return <{k} {k}={k} {"k"}={k + "world"}><{k + "k"}/></{k}>;}';
TEST(7, expect, actual);

END();

function ToString(f)
{
  return f.toString().replace(/\n/g, '').replace(/[ ]+/g, ' ');
}
