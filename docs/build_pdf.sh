#!/bin/sh
# Rebuilds docs/PROPOSAL_ANALYSIS.pdf from the Markdown source.
# Needs pandoc and Google Chrome (or Chromium). Run from the repository root:
#   sh docs/build_pdf.sh
set -e
cd "$(dirname "$0")"
pandoc PROPOSAL_ANALYSIS.md -s --embed-resources --css pdf.css -o /tmp/proposal_analysis.html
CHROME=$(command -v google-chrome || command -v chromium || command -v chromium-browser)
"$CHROME" --headless --disable-gpu --no-sandbox --no-pdf-header-footer \
  --print-to-pdf="$PWD/PROPOSAL_ANALYSIS.pdf" /tmp/proposal_analysis.html
echo "Written docs/PROPOSAL_ANALYSIS.pdf"
