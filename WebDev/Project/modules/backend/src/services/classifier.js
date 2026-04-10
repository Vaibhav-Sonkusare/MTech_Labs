/**
 * Advanced Heuristic Rule-based website classifier.
 * Includes Smart Regex matching to dynamically override site domains via Title context.
 */

// ─── Domain → Category rules ───
const DOMAIN_RULES = {
  // Productive
  'github.com': { category: 'productive', confidence: 0.95 },
  'stackoverflow.com': { category: 'productive', confidence: 0.92 },
  'docs.python.org': { category: 'productive', confidence: 0.92 },
  'developer.mozilla.org': { category: 'productive', confidence: 0.95 },
  'figma.com': { category: 'productive', confidence: 0.88 },
  'notion.so': { category: 'productive', confidence: 0.85 },
  'trello.com': { category: 'productive', confidence: 0.85 },
  'jira.atlassian.com': { category: 'productive', confidence: 0.90 },
  'confluence.atlassian.com': { category: 'productive', confidence: 0.88 },
  'linear.app': { category: 'productive', confidence: 0.88 },
  'vercel.com': { category: 'productive', confidence: 0.88 },
  'aws.amazon.com': { category: 'productive', confidence: 0.90 },
  'cloud.google.com': { category: 'productive', confidence: 0.90 },
  'docs.google.com': { category: 'productive', confidence: 0.85 },
  'leetcode.com': { category: 'productive', confidence: 0.88 },
  
  // Learning
  'coursera.org': { category: 'learning', confidence: 0.95 },
  'udemy.com': { category: 'learning', confidence: 0.95 },
  'edx.org': { category: 'learning', confidence: 0.95 },
  'khanacademy.org': { category: 'learning', confidence: 0.95 },
  'freecodecamp.org': { category: 'learning', confidence: 0.92 },
  'medium.com': { category: 'learning', confidence: 0.70 },
  'dev.to': { category: 'learning', confidence: 0.80 },
  'arxiv.org': { category: 'learning', confidence: 0.92 },
  'wikipedia.org': { category: 'learning', confidence: 0.75 },
  
  // Distracting
  'youtube.com': { category: 'distracting', confidence: 0.80 }, // Very susceptible to title overrides
  'facebook.com': { category: 'distracting', confidence: 0.92 },
  'instagram.com': { category: 'distracting', confidence: 0.92 },
  'twitter.com': { category: 'distracting', confidence: 0.88 },
  'x.com': { category: 'distracting', confidence: 0.88 },
  'tiktok.com': { category: 'distracting', confidence: 0.95 },
  'reddit.com': { category: 'distracting', confidence: 0.80 },
  'twitch.tv': { category: 'distracting', confidence: 0.90 },
  'netflix.com': { category: 'distracting', confidence: 0.95 },
  'primevideo.com': { category: 'distracting', confidence: 0.95 },
  'discord.com': { category: 'distracting', confidence: 0.78 },
  'whatsapp.com': { category: 'distracting', confidence: 0.80 },
  
  // Neutral
  'google.com': { category: 'neutral', confidence: 0.60 },
  'bing.com': { category: 'neutral', confidence: 0.60 },
};

// ─── Regex Keyword Dictionaries ───
// Using \b word boundaries to avoid matching substrings
const TITLE_HEURISTICS = {
  productive: [
    /pull request/i, /merge/i, /deploy/i, /pipeline/i, /\bdebug\b/i,
    /refactor/i, /code review/i, /sprint/i, /standup/i, /kanban/i,
    /backlog/i, /\bapi\b/i, /database/i, /server/i, /devops/i,
    /ci\/cd/i, /docker/i, /kubernetes/i, /terraform/i, /architecture/i,
    /design system/i, /wireframe/i, /prototype/i, /dashboard/i,
    /analytics/i, /monitoring/i, /performance/i, /documentation/i,
    /\bdocs\b/i, /roadmap/i, /issue(s)?/i, /ticket(s)?/i,
    /localhost:?\d*/i, /staging/i, /production/i, /admin/i,
    /editor/i, /IDE/i, /console/i,
  ],
  learning: [
    /\btutorial(s)?\b/i, /\bcourse(s)?\b/i, /\blecture(s)?\b/i, /\blesson(s)?\b/i,
    /\blearn\b/i, /\blearning\b/i, /\btraining\b/i, /\bworkshop\b/i, 
    /\bbootcamp\b/i, /\bcertification\b/i, /how to/i, /\bguide(s)?\b/i,
    /introduction to/i, /\bbeginner(s)?\b/i, /\badvanced\b/i, /masterclass/i,
    /\bexplain(ed)?\b/i, /understand(ing)?/i, /\bstudy(ing)?\b/i, /\bresearch\b/i,
    /\bpaper\b/i, /\bthesis\b/i, /\bjournal\b/i, /\balgorithm(s)?\b/i,
    /data structure(s)?/i, /machine learning/i, /deep learning/i, 
    /artificial intelligence/i, /neural network/i, /programming/i,
    /coding tutorial/i, /web development/i, /full(\-)?stack/i,
    /crash course/i, /syllabus/i, /mastering/i, /101\b/i, /cs50/i,
    /\bmath(ematics)?\b/i, /\bphysics\b/i, /\bscience\b/i, /\bchemistry\b/i,
    /\bbiology\b/i, /history of/i, /philosophy/i,
  ],
  distracting: [
    /\bfunny\b/i, /\bmeme(s)?\b/i, /\bprank(s)?\b/i, /\bfail(s)?\b/i, 
    /compilation/i, /\breaction(s)?\b/i, /\bvlog(s)?\b/i, /unboxing/i,
    /\bhaul(s)?\b/i, /\bdrama\b/i, /\bgossip\b/i, /celebrity/i,
    /\bviral\b/i, /trending/i, /\bshorts\b/i, /\breels\b/i, /\btiktok(s)?\b/i,
    /gameplay/i, /gaming/i, /playthrough/i, /walkthrough/i,
    /\bstream\b/i, /live stream/i, /highlight(s)?\b/i,
    /\bmovie(s)?\b/i, /\btrailer(s)?\b/i, /\bepisode(s)?\b/i, 
    /\bseries\b/i, /\bseason\b/i, /music video/i, /\bsong(s)?\b/i, 
    /lyrics/i, /\balbum(s)?\b/i, /let'?s play/i, /top 10/i, /try not to/i,
    /funny moments/i, /bloopers/i,
  ]
};

// ─── Matching logic ───
function matchDomain(domain) {
  let d = domain.toLowerCase().trim();
  if (d.startsWith('www.')) d = d.slice(4);

  // Exact match
  if (DOMAIN_RULES[d]) {
    return { category: DOMAIN_RULES[d].category, confidence: DOMAIN_RULES[d].confidence };
  }

  // Suffix match (e.g. "docs.github.com" → "github.com")
  for (const [ruleDomain, rule] of Object.entries(DOMAIN_RULES)) {
    if (d.endsWith('.' + ruleDomain) || ruleDomain.endsWith('.' + d)) {
      return { category: rule.category, confidence: rule.confidence - 0.05 };
    }
  }
  return null;
}

function matchTitleRegex(title) {
  const t = title.trim();
  if (!t) return null;

  const hits = { productive: 0, learning: 0, distracting: 0 };

  for (const [category, regexes] of Object.entries(TITLE_HEURISTICS)) {
    for (const regex of regexes) {
      if (regex.test(t)) {
        hits[category] += 1;
      }
    }
  }

  // Find category with most hits
  const bestCategory = Object.keys(hits).reduce((a, b) => 
    hits[a] > hits[b] ? a : b
  );

  if (hits[bestCategory] === 0) return null;
  
  // Calculate confidence based on hits. 
  const confidence = Math.min(0.70 + ((hits[bestCategory] - 1) * 0.15), 0.95);

  return { category: bestCategory, confidence, hits: hits[bestCategory] };
}

// ─── Public API ───
exports.classify = function classify(domain, title) {
  const domainRes = matchDomain(domain);
  const titleRes = matchTitleRegex(title);

  // Default fallback
  const fallback = { category: 'neutral', confidence: 0.50 };

  if (!domainRes && !titleRes) return fallback;
  if (domainRes && !titleRes) return domainRes;
  if (!domainRes && titleRes) return { category: titleRes.category, confidence: titleRes.confidence };

  // Smart Overrides!
  // If the domain is distracting/neutral but the title clearly indicates learning/productive
  if (titleRes.category !== domainRes.category) {
    if ((titleRes.category === 'learning' || titleRes.category === 'productive') 
         && (domainRes.category === 'distracting' || domainRes.category === 'neutral')) {
      return { 
        category: titleRes.category, 
        // Force very high confidence to explicitly override UI defaults
        confidence: Math.max(0.95, titleRes.confidence) 
      };
    }
    
    // If title reveals it's distracting even on a learning site
    if (titleRes.category === 'distracting' && titleRes.hits >= 2) {
       return { category: 'distracting', confidence: 0.80 };
    }
  }

  return domainRes;
};

exports.classifyWithOverrides = async function classifyWithOverrides(domain, title, userId, prisma) {
  let d = domain.toLowerCase().trim();
  if (d.startsWith('www.')) d = d.slice(4);

  if (userId && prisma) {
    const custom = await prisma.customCategory.findUnique({
      where: { userId_domain: { userId, domain: d } },
    });
    if (custom) {
      return { category: custom.category, confidence: 1.0 };
    }
  }

  return exports.classify(domain, title);
};
