# Dev → People

*Page and controls* · joshuavelasco.com/dev/#people · [all pages](README.md)

Who may come into the bar: the owner, admins, invited players, people waiting.

| Control | What it does |
|---|---|
| `Invite` | Lets them in. |
| `Their username` | The account to invite. |
| `Make a new Claude key` | Owner only, under the list (Claude's key): asks first, then makes a new key for Claude's sessions (`hub/hub.php`, `{"a":"key"}`); the old key stops working. The key shows once in a field with a Copy button (drawn only then); it goes in the Claude app's environment settings as `HUB_KEY`, never in the chat. |

Each person's row has its own buttons (Let in, Make admin, Remove).
