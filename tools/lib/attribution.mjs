// The lines that end every commit message (restructure J4.3, KI-15): what the
// session gives in PIECE_ATTRIBUTION (exactly as the session's instructions
// say), else built from this session's own id; never an old session's.
export function attribution() {
  if (process.env.PIECE_ATTRIBUTION) return process.env.PIECE_ATTRIBUTION;
  const id = (process.env.CLAUDE_CODE_REMOTE_SESSION_ID || '').replace(/^cse_/, '');
  return [`Co-Authored-By: ${process.env.CLAUDE_COAUTHOR || 'Claude <noreply@anthropic.com>'}`,
          ...(id ? [`Claude-Session: https://claude.ai/code/session_${id}`] : [])].join('\n');
}
