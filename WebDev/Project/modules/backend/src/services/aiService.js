// Mock AI classification service

exports.classifyActivity = async (domain, title) => {
  // Very simple mock logic
  if (domain.includes('youtube')) {
    return { category: 'distracting', confidence: 0.85 };
  }

  if (domain.includes('github') || domain.includes('stackoverflow')) {
    return { category: 'productive', confidence: 0.92 };
  }

  return { category: 'neutral', confidence: 0.60 };
};
