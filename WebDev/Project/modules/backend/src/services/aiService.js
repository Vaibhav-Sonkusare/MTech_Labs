const classifier = require('./classifier');

/**
 * Classify browsing activity using the built-in rule-based classifier.
 */
exports.classifyActivity = async (domain, title) => {
  return classifier.classify(domain, title);
};

/**
 * Classify with user-specific overrides (checks DB first).
 */
exports.classifyWithOverrides = async (domain, title, userId, prisma) => {
  return classifier.classifyWithOverrides(domain, title, userId, prisma);
};
